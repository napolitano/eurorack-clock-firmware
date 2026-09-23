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
#include "generated_panel_layout.hpp"

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
        OUT1_LIGHT,
        OUT2_LIGHT,
        OUT3_LIGHT,
        OUT4_LIGHT,
        OUT5_LIGHT,
        OUT6_LIGHT,
        OUT7_LIGHT,
        OUT8_LIGHT,
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
        configButton(TAP_PARAM, "TAP / SHIFT");
        configButton(STOP_PARAM, "STOP / BACK");
        configInput(SYNC_INPUT, "IN 1 / SYNC");
        configInput(RESET_INPUT, "IN 2 / RST");
        for (int index = 0; index < 8; ++index) {
            configOutput(OUT1_OUTPUT + index, std::to_string(index + 1));
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
            const float gateVoltage = runtime->gateVoltage(static_cast<std::size_t>(index));
            outputs[OUT1_OUTPUT + index].setVoltage(gateVoltage);
            lights[OUT1_LIGHT + index].setBrightness(gateVoltage > 0.0F ? 1.0F : 0.0F);
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

struct ClockEncoderKnob final : app::SvgKnob {
    ClockEncoderKnob() {
        constexpr float kPi = 3.14159265358979323846F;
        minAngle = -0.83F * kPi;
        maxAngle = 0.83F * kPi;
        snap = true;
        setSvg(window::Svg::load(asset::plugin(pluginInstance, "res/encoder.svg")));
    }
};

/**
 * @brief Transparent centre switch that models the encoder's physical push shaft.
 *
 * The outer encoder ring remains a Rack knob for rotation. Holding the centre keeps the real
 * encoder-button level asserted, so firmware short-press, long-press and chord timing use their
 * production debounce/gesture implementation instead of a synthetic fixed-duration click.
 */
struct ClockEncoderPushButton final : app::SvgSwitch {
    ClockEncoderPushButton() {
        momentary = true;
        addFrame(window::Svg::load(asset::plugin(pluginInstance, "res/encoder-push-0.svg")));
        addFrame(window::Svg::load(asset::plugin(pluginInstance, "res/encoder-push-1.svg")));
    }
};

struct ClockPlayButton final : app::SvgSwitch {
    ClockPlayButton() {
        momentary = true;
        addFrame(window::Svg::load(asset::plugin(pluginInstance, "res/button-play-0.svg")));
        addFrame(window::Svg::load(asset::plugin(pluginInstance, "res/button-play-1.svg")));
    }
};

struct ClockTapButton final : app::SvgSwitch {
    ClockTapButton() {
        momentary = true;
        addFrame(window::Svg::load(asset::plugin(pluginInstance, "res/button-tap-0.svg")));
        addFrame(window::Svg::load(asset::plugin(pluginInstance, "res/button-tap-1.svg")));
    }
};

struct ClockStopButton final : app::SvgSwitch {
    ClockStopButton() {
        momentary = true;
        addFrame(window::Svg::load(asset::plugin(pluginInstance, "res/button-stop-0.svg")));
        addFrame(window::Svg::load(asset::plugin(pluginInstance, "res/button-stop-1.svg")));
    }
};

struct ClockWidget final : ModuleWidget {
    static Vec panelPoint(const clockfw::vcv::panel::PointMm& point) {
        using namespace clockfw::vcv::panel;
        return mm2px(Vec(point.x + kPanelOffsetXmm, point.y + kPanelOffsetYmm));
    }

    explicit ClockWidget(ClockModule* module) {
        using namespace clockfw::vcv::panel;

        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/CLOCK.svg")));

        auto* display = createWidget<ClockDisplayWidget>(
            mm2px(Vec(kDisplay.x + kPanelOffsetXmm, kDisplay.y + kPanelOffsetYmm)));
        display->box.size = mm2px(Vec(kDisplay.width, kDisplay.height));
        display->module = module;
        addChild(display);

        addParam(createParamCentered<ClockEncoderKnob>(
            panelPoint(kEncoderCenter), module, ClockModule::ENCODER_PARAM));
        addParam(createParamCentered<ClockEncoderPushButton>(
            panelPoint(kEncoderCenter), module, ClockModule::ENCODER_PUSH_PARAM));

        addParam(createParamCentered<ClockPlayButton>(panelPoint(kPlayCenter), module, ClockModule::PLAY_PARAM));
        addParam(createParamCentered<ClockTapButton>(panelPoint(kTapCenter), module, ClockModule::TAP_PARAM));
        addParam(createParamCentered<ClockStopButton>(panelPoint(kStopCenter), module, ClockModule::STOP_PARAM));

        addInput(createInputCentered<PJ301MPort>(panelPoint(kSyncCenter), module, ClockModule::SYNC_INPUT));
        addInput(createInputCentered<PJ301MPort>(panelPoint(kResetCenter), module, ClockModule::RESET_INPUT));

        for (int index = 0; index < 8; ++index) {
            const std::size_t position = static_cast<std::size_t>(index);
            addChild(createLightCentered<MediumLight<RedLight>>(
                panelPoint(kLedCenters[position]), module, ClockModule::OUT1_LIGHT + index));
            addOutput(createOutputCentered<PJ301MPort>(
                panelPoint(kOutputCenters[position]), module, ClockModule::OUT1_OUTPUT + index));
        }

        for (const PointMm& screw : kScrewCenters) {
            addChild(createWidgetCentered<ScrewSilver>(panelPoint(screw)));
        }
    }
};

Model* modelClock = createModel<ClockModule, ClockWidget>("CLOCK");
