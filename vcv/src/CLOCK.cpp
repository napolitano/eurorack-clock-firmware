/**
 * @file CLOCK.cpp
 * @brief Functional VCV Rack shell around the real South Signal Lab CLOCK runtime.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "plugin.hpp"

#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

#include "clock_vcv_runtime.h"

namespace {

std::atomic<bool> gHostRuntimeClaimed{false};

std::string encodeHex(const std::uint8_t* data, const std::size_t size) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string encoded;
    encoded.resize(size * 2U);
    for (std::size_t index = 0U; index < size; ++index) {
        encoded[index * 2U] = kHex[(data[index] >> 4U) & 0x0FU];
        encoded[index * 2U + 1U] = kHex[data[index] & 0x0FU];
    }
    return encoded;
}

bool decodeNibble(const char value, std::uint8_t& nibble) {
    if (value >= '0' && value <= '9') {
        nibble = static_cast<std::uint8_t>(value - '0');
        return true;
    }
    if (value >= 'a' && value <= 'f') {
        nibble = static_cast<std::uint8_t>(10 + value - 'a');
        return true;
    }
    if (value >= 'A' && value <= 'F') {
        nibble = static_cast<std::uint8_t>(10 + value - 'A');
        return true;
    }
    return false;
}

template <std::size_t Size>
bool decodeHex(const std::string& encoded, std::array<std::uint8_t, Size>& decoded) {
    if (encoded.size() != Size * 2U) {
        return false;
    }
    for (std::size_t index = 0U; index < Size; ++index) {
        std::uint8_t high = 0U;
        std::uint8_t low = 0U;
        if (!decodeNibble(encoded[index * 2U], high) ||
            !decodeNibble(encoded[index * 2U + 1U], low)) {
            return false;
        }
        decoded[index] = static_cast<std::uint8_t>((high << 4U) | low);
    }
    return true;
}

std::filesystem::path temporaryStatePath(const void* identity) {
    return std::filesystem::temp_directory_path() /
        ("south-signal-lab-clock-vcv-" +
         std::to_string(reinterpret_cast<std::uintptr_t>(identity)) + ".bin");
}

}  // namespace

struct ClockModule final : Module {
    enum ParamId {
        ENCODER_PARAM,
        ENCODER_PUSH_PARAM,
        PLAY_PARAM,
        TAP_PARAM,
        STOP_PARAM,
        PARAMS_LEN
    };

    enum InputId {
        SYNC_INPUT,
        RESET_INPUT,
        INPUTS_LEN
    };

    enum OutputId {
        OUT1_OUTPUT,
        OUT2_OUTPUT,
        OUT3_OUTPUT,
        OUT4_OUTPUT,
        OUT5_OUTPUT,
        OUT6_OUTPUT,
        OUT7_OUTPUT,
        OUT8_OUTPUT,
        OUTPUTS_LEN
    };

    enum LightId {
        LIGHTS_LEN
    };

    std::unique_ptr<clockfw::vcv::ClockVcvRuntime> runtime{};
    std::filesystem::path statePath{};
    std::array<std::atomic<std::uint8_t>, clockfw::hal::OledDisplay::kFramebufferSize> display{};
    int lastEncoderDetent = 0;
    std::uint32_t displayDivider = 0U;
    bool ownsHostRuntime = false;

    ClockModule() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(ENCODER_PARAM, -128.0F, 128.0F, 0.0F, "Encoder");
        getParamQuantity(ENCODER_PARAM)->snapEnabled = true;
        getParamQuantity(ENCODER_PARAM)->resetEnabled = false;
        getParamQuantity(ENCODER_PARAM)->randomizeEnabled = false;
        configButton(ENCODER_PUSH_PARAM, "Encoder push");
        configButton(PLAY_PARAM, "PLAY / PAUSE");
        configButton(TAP_PARAM, "TAP");
        configButton(STOP_PARAM, "STOP / BACK");
        configInput(SYNC_INPUT, "SYNC");
        configInput(RESET_INPUT, "RST");
        for (int index = 0; index < 8; ++index) {
            configOutput(OUT1_OUTPUT + index, "Gate " + std::to_string(index + 1));
        }
        for (auto& byte : display) {
            byte.store(0U, std::memory_order_relaxed);
        }

        bool expectedUnclaimed = false;
        ownsHostRuntime = gHostRuntimeClaimed.compare_exchange_strong(
            expectedUnclaimed, true, std::memory_order_acq_rel);
        if (ownsHostRuntime) {
            statePath = temporaryStatePath(this);
            std::error_code ignored;
            std::filesystem::remove(statePath, ignored);
            runtime = std::make_unique<clockfw::vcv::ClockVcvRuntime>(statePath);
            runtime->begin();
            publishDisplay();
        }
    }

    ~ClockModule() override {
        runtime.reset();
        if (!statePath.empty()) {
            std::error_code ignored;
            std::filesystem::remove(statePath, ignored);
        }
        if (ownsHostRuntime) {
            gHostRuntimeClaimed.store(false, std::memory_order_release);
        }
    }

    void process(const ProcessArgs& args) override {
        if (!runtime) {
            for (int index = 0; index < 8; ++index) {
                outputs[OUT1_OUTPUT + index].setVoltage(0.0F);
            }
            return;
        }

        const int encoderDetent = static_cast<int>(std::lround(params[ENCODER_PARAM].getValue()));
        if (encoderDetent != lastEncoderDetent) {
            runtime->rotateEncoder(encoderDetent - lastEncoderDetent);
            lastEncoderDetent = encoderDetent;
        }
        if (encoderDetent >= 120 || encoderDetent <= -120) {
            params[ENCODER_PARAM].setValue(0.0F);
            lastEncoderDetent = 0;
        }

        clockfw::vcv::PanelControls controls{};
        controls.encoderPressed = params[ENCODER_PUSH_PARAM].getValue() >= 0.5F;
        controls.playPressed = params[PLAY_PARAM].getValue() >= 0.5F;
        controls.tapPressed = params[TAP_PARAM].getValue() >= 0.5F;
        controls.stopPressed = params[STOP_PARAM].getValue() >= 0.5F;
        runtime->setPanelControls(controls);

        const bool syncConnected = inputs[SYNC_INPUT].isConnected();
        const bool resetConnected = inputs[RESET_INPUT].isConnected();
        runtime->processSample(
            static_cast<double>(args.sampleTime),
            syncConnected,
            syncConnected ? inputs[SYNC_INPUT].getVoltage() : 0.0F,
            resetConnected,
            resetConnected ? inputs[RESET_INPUT].getVoltage() : 0.0F);

        for (int index = 0; index < 8; ++index) {
            outputs[OUT1_OUTPUT + index].setVoltage(
                runtime->gateVoltage(static_cast<std::size_t>(index)));
        }

        ++displayDivider;
        const std::uint32_t refreshSamples = static_cast<std::uint32_t>(
            std::max(1.0F, args.sampleRate / 200.0F));
        if (displayDivider >= refreshSamples) {
            displayDivider = 0U;
            publishDisplay();
        }
    }

    json_t* dataToJson() override {
        json_t* root = json_object();
        if (!runtime) {
            json_object_set_new(root, "runtimeAvailable", json_false());
            return root;
        }
        const auto image = runtime->persistenceImage();
        const std::string encoded = encodeHex(image.data(), image.size());
        json_object_set_new(root, "clockPersistenceHex", json_stringn(encoded.data(), encoded.size()));
        json_object_set_new(root, "engineBaseline", json_string("1.1.0"));
        return root;
    }

    void dataFromJson(json_t* root) override {
        if (!runtime || root == nullptr) {
            return;
        }
        json_t* encodedJson = json_object_get(root, "clockPersistenceHex");
        if (encodedJson == nullptr || !json_is_string(encodedJson)) {
            return;
        }
        const char* encodedChars = json_string_value(encodedJson);
        if (encodedChars == nullptr) {
            return;
        }
        std::array<std::uint8_t, clockfw::hal::PersistentStorage::kCapacityBytes> image{};
        if (!decodeHex(std::string(encodedChars), image)) {
            WARN("SouthSignalLab-CLOCK: rejected malformed persistence image in Rack patch");
            return;
        }
        runtime->restorePersistenceImage(image);
        publishDisplay();
    }

    void publishDisplay() {
        if (!runtime) {
            return;
        }
        const auto& frame = runtime->framebuffer();
        for (std::size_t index = 0U; index < frame.size(); ++index) {
            display[index].store(frame[index], std::memory_order_relaxed);
        }
    }
};

struct ClockDisplayWidget final : Widget {
    ClockModule* module = nullptr;

    void draw(const DrawArgs& args) override {
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0.0F, 0.0F, box.size.x, box.size.y, 3.0F);
        nvgFillColor(args.vg, nvgRGB(4, 8, 12));
        nvgFill(args.vg);

        if (module == nullptr || !module->runtime) {
            nvgFontSize(args.vg, 10.0F);
            nvgFillColor(args.vg, nvgRGB(220, 220, 220));
            nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgText(args.vg, box.size.x * 0.5F, box.size.y * 0.5F,
                module == nullptr ? "CLOCK" : "ONE INSTANCE", nullptr);
            return;
        }

        const float scaleX = box.size.x / 128.0F;
        const float scaleY = box.size.y / 64.0F;
        nvgFillColor(args.vg, nvgRGB(245, 245, 245));
        for (std::size_t y = 0U; y < 64U; ++y) {
            const std::size_t page = y / 8U;
            const std::uint8_t mask = static_cast<std::uint8_t>(1U << (y & 7U));
            for (std::size_t x = 0U; x < 128U; ++x) {
                const std::size_t index = page * 128U + x;
                if ((module->display[index].load(std::memory_order_relaxed) & mask) == 0U) {
                    continue;
                }
                nvgBeginPath(args.vg);
                nvgRect(args.vg,
                    static_cast<float>(x) * scaleX,
                    static_cast<float>(y) * scaleY,
                    std::max(1.0F, scaleX),
                    std::max(1.0F, scaleY));
                nvgFill(args.vg);
            }
        }
    }
};

struct ClockWidget final : ModuleWidget {
    explicit ClockWidget(ClockModule* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/CLOCK.svg")));

        auto* display = createWidget<ClockDisplayWidget>(Vec(11.0F, 31.0F));
        display->box.size = Vec(128.0F, 64.0F);
        display->module = module;
        addChild(display);

        addParam(createParamCentered<RoundBlackKnob>(Vec(75.0F, 129.0F), module, ClockModule::ENCODER_PARAM));
        addParam(createParamCentered<LEDButton>(Vec(75.0F, 154.0F), module, ClockModule::ENCODER_PUSH_PARAM));

        addParam(createParamCentered<LEDButton>(Vec(35.0F, 190.0F), module, ClockModule::PLAY_PARAM));
        addParam(createParamCentered<LEDButton>(Vec(75.0F, 190.0F), module, ClockModule::TAP_PARAM));
        addParam(createParamCentered<LEDButton>(Vec(115.0F, 190.0F), module, ClockModule::STOP_PARAM));

        addInput(createInputCentered<PJ301MPort>(Vec(38.0F, 229.0F), module, ClockModule::SYNC_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(112.0F, 229.0F), module, ClockModule::RESET_INPUT));

        constexpr float kOutputX[2] = {38.0F, 112.0F};
        constexpr float kOutputY[4] = {265.0F, 298.0F, 331.0F, 364.0F};
        int outputIndex = 0;
        for (int row = 0; row < 4; ++row) {
            for (int column = 0; column < 2; ++column) {
                addOutput(createOutputCentered<PJ301MPort>(
                    Vec(kOutputX[column], kOutputY[row]), module,
                    ClockModule::OUT1_OUTPUT + outputIndex));
                ++outputIndex;
            }
        }
    }
};

Model* modelClock = createModel<ClockModule, ClockWidget>("CLOCK");
