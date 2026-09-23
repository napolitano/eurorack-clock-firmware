/**
 * @file CLOCK.cpp
 * @brief Functional VCV Rack shell around the real South Signal Lab CLOCK runtime.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "plugin.hpp"

#include <algorithm>
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
    enum class EncoderGestureMode : std::uint8_t {
        Idle,
        Pending,
        Press,
        Rotate
    };

    std::atomic<int> queuedEncoderDetents{0};
    std::atomic<EncoderGestureMode> encoderGestureMode{EncoderGestureMode::Idle};
    std::atomic<bool> encoderShortClickReleased{false};
    std::atomic<bool> generalSettingsRequested{false};
    std::atomic<bool> keyboardLeftShiftHeld{false};
    std::atomic<bool> keyboardRightShiftHeld{false};
    std::atomic<bool> keyboardTapHeld{false};
    std::atomic<bool> keyboardEncoderHeld{false};
    std::atomic<bool> keyboardPlayHeld{false};
    std::atomic<bool> keyboardStopHeld{false};
    double encoderPendingSeconds = 0.0;
    double encoderClickPulseSeconds = 0.0;
    std::uint32_t displayDivider = 0U;
    bool ownsHostRuntime = false;

    ClockModule() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        // Legacy hidden parameter IDs are retained so early experimental Rack patches keep
        // their parameter indexing. The interactive endless encoder is handled directly by the
        // custom widget below rather than by a bounded Rack ParamQuantity.
        configParam(ENCODER_PARAM, -128.0F, 128.0F, 0.0F, "Encoder (legacy hidden)");
        getParamQuantity(ENCODER_PARAM)->resetEnabled = false;
        getParamQuantity(ENCODER_PARAM)->randomizeEnabled = false;
        configButton(ENCODER_PUSH_PARAM, "Encoder push (legacy hidden)");
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

    void queueEncoderDetents(const int detents) noexcept {
        if (detents != 0) {
            queuedEncoderDetents.fetch_add(detents, std::memory_order_relaxed);
        }
    }

    void beginEncoderGesture() noexcept {
        encoderGestureMode.store(EncoderGestureMode::Pending, std::memory_order_release);
    }

    bool beginEncoderRotation() noexcept {
        EncoderGestureMode expected = EncoderGestureMode::Pending;
        if (encoderGestureMode.compare_exchange_strong(
                expected,
                EncoderGestureMode::Rotate,
                std::memory_order_acq_rel,
                std::memory_order_acquire)) {
            return true;
        }
        return expected == EncoderGestureMode::Rotate;
    }

    void requestGeneralSettings() noexcept {
        generalSettingsRequested.store(true, std::memory_order_release);
    }

    void setKeyboardShiftHeld(const int key, const bool held) noexcept {
        if (key == GLFW_KEY_LEFT_SHIFT) {
            keyboardLeftShiftHeld.store(held, std::memory_order_release);
        } else if (key == GLFW_KEY_RIGHT_SHIFT) {
            keyboardRightShiftHeld.store(held, std::memory_order_release);
        }
    }

    void setKeyboardTapHeld(const bool held) noexcept {
        keyboardTapHeld.store(held, std::memory_order_release);
    }

    void setKeyboardEncoderHeld(const bool held) noexcept {
        keyboardEncoderHeld.store(held, std::memory_order_release);
    }

    void setKeyboardPlayHeld(const bool held) noexcept {
        keyboardPlayHeld.store(held, std::memory_order_release);
    }

    void setKeyboardStopHeld(const bool held) noexcept {
        keyboardStopHeld.store(held, std::memory_order_release);
    }

    void endEncoderGesture() noexcept {
        const EncoderGestureMode finished = encoderGestureMode.exchange(
            EncoderGestureMode::Idle, std::memory_order_acq_rel);
        if (finished == EncoderGestureMode::Pending) {
            encoderShortClickReleased.store(true, std::memory_order_release);
        }
    }

    void process(const ProcessArgs& args) override {
        if (!runtime) {
            for (int index = 0; index < 8; ++index) {
                outputs[OUT1_OUTPUT + index].setVoltage(0.0F);
            }
            return;
        }

        if (generalSettingsRequested.exchange(false, std::memory_order_acq_rel)) {
            if (!runtime->openGeneralSettings()) {
                // A menu command issued during the one-second boot remains pending until the
                // normal CLOCK application reaches its running UI state.
                generalSettingsRequested.store(true, std::memory_order_release);
            }
        }

        const int encoderDetents = queuedEncoderDetents.exchange(0, std::memory_order_acq_rel);
        if (encoderDetents != 0) {
            runtime->rotateEncoder(encoderDetents);
        }

        // One physical control supports two mutually exclusive gestures. A newly pressed encoder
        // stays pending while the UI determines whether the user starts a vertical drag. Movement
        // classifies the complete gesture as rotation; a stationary hold becomes push only after a
        // deliberate grace period. Once classified, a gesture never changes type until release.
        constexpr double kEncoderPushArmSeconds = 0.180;
        constexpr double kEncoderClickPulseSeconds = 0.040;
        EncoderGestureMode encoderMode = encoderGestureMode.load(std::memory_order_acquire);
        if (encoderMode == EncoderGestureMode::Pending) {
            encoderPendingSeconds += static_cast<double>(args.sampleTime);
            if (encoderPendingSeconds >= kEncoderPushArmSeconds) {
                EncoderGestureMode expected = EncoderGestureMode::Pending;
                if (encoderGestureMode.compare_exchange_strong(
                        expected,
                        EncoderGestureMode::Press,
                        std::memory_order_acq_rel,
                        std::memory_order_acquire)) {
                    encoderMode = EncoderGestureMode::Press;
                } else {
                    encoderMode = expected;
                }
            }
        } else {
            encoderPendingSeconds = 0.0;
        }

        if (encoderShortClickReleased.exchange(false, std::memory_order_acq_rel)) {
            encoderClickPulseSeconds = kEncoderClickPulseSeconds;
        }

        bool encoderPressed = encoderMode == EncoderGestureMode::Press ||
            keyboardEncoderHeld.load(std::memory_order_acquire);
        if (encoderClickPulseSeconds > 0.0) {
            encoderPressed = true;
            encoderClickPulseSeconds = std::max(
                0.0, encoderClickPulseSeconds - static_cast<double>(args.sampleTime));
        }

        clockfw::vcv::PanelControls controls{};
        controls.encoderPressed = encoderPressed;
        controls.playPressed = params[PLAY_PARAM].getValue() >= 0.5F ||
            keyboardPlayHeld.load(std::memory_order_acquire);
        controls.tapPressed = params[TAP_PARAM].getValue() >= 0.5F ||
            keyboardLeftShiftHeld.load(std::memory_order_acquire) ||
            keyboardRightShiftHeld.load(std::memory_order_acquire) ||
            keyboardTapHeld.load(std::memory_order_acquire);
        controls.stopPressed = params[STOP_PARAM].getValue() >= 0.5F ||
            keyboardStopHeld.load(std::memory_order_acquire);
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
            const std::size_t channel = static_cast<std::size_t>(index);
            const float gateVoltage = runtime->gateVoltage(channel);
            const bool gateHigh = gateVoltage > 0.0F;
            outputs[OUT1_OUTPUT + index].setVoltage(gateVoltage);

            // Rack's engine light smoothing gives short gates an immediate visible attack and a
            // bounded decay, without a separate hold timer that can make activity appear latched.
            // The actual output voltage above remains the instantaneous production gate state.
            lights[OUT1_LIGHT + index].setBrightnessSmooth(
                gateHigh ? 1.0F : 0.0F, args.sampleTime);
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

/**
 * @brief Draws all panel lettering with Rack's UI font.
 *
 * Rack's NanoSVG renderer intentionally does not render SVG <text> elements. The generated SVG
 * keeps text for standalone previews/documentation, while this overlay makes the same labels
 * visible in the actual Rack module without bundling a second font.
 */
struct ClockPanelLabels final : Widget {
    static Vec panelPoint(const clockfw::vcv::panel::PointMm& point) {
        using namespace clockfw::vcv::panel;
        return mm2px(Vec(point.x + kPanelOffsetXmm, point.y + kPanelOffsetYmm));
    }

    static Vec panelPoint(const float xMm, const float yMm) {
        using namespace clockfw::vcv::panel;
        return mm2px(Vec(xMm + kPanelOffsetXmm, yMm + kPanelOffsetYmm));
    }

    static void drawLabel(
        NVGcontext* vg,
        const Vec& position,
        const char* text,
        const float size,
        const NVGcolor color,
        const int align = NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE) {
        if (APP == nullptr || APP->window == nullptr || !APP->window->uiFont) {
            return;
        }
        nvgFontFaceId(vg, APP->window->uiFont->handle);
        nvgFontSize(vg, size);
        nvgFillColor(vg, color);
        nvgTextAlign(vg, align);
        nvgText(vg, position.x, position.y, text, nullptr);
    }

    void draw(const DrawArgs& args) override {
        using namespace clockfw::vcv::panel;
        constexpr float panelCenterX = kPanelWidthMm * 0.5F;

        const NVGcolor primary = nvgRGB(244, 244, 244);
        const NVGcolor secondary = nvgRGB(142, 144, 148);

        // The CLOCK wordmark itself is project-owned vector geometry in the generated panel SVG.
        // Control labels always sit below their hardware. The encoder deliberately remains
        // unlabelled, because the OLED UI carries its context and the panel benefits from space.
        const float buttonLabelY = kPlayCenter.y + kButtonActuatorDiameterMm * 0.5F + 1.25F;
        constexpr float labelLineGapMm = 2.25F;
        drawLabel(args.vg, panelPoint(kPlayCenter.x, buttonLabelY), "PLAY", 7.2F, primary);
        drawLabel(args.vg, panelPoint(kPlayCenter.x, buttonLabelY + labelLineGapMm), "PAUSE", 6.5F, secondary);
        drawLabel(args.vg, panelPoint(kTapCenter.x, buttonLabelY), "TAP", 7.2F, primary);
        drawLabel(args.vg, panelPoint(kTapCenter.x, buttonLabelY + labelLineGapMm), "SHIFT", 6.5F, secondary);
        drawLabel(args.vg, panelPoint(kStopCenter.x, buttonLabelY), "STOP", 7.2F, primary);
        drawLabel(args.vg, panelPoint(kStopCenter.x, buttonLabelY + labelLineGapMm), "BACK", 6.5F, secondary);

        const float inputLabelY = kSyncCenter.y + kJackNutDiameterMm * 0.5F + 2.4F;
        drawLabel(args.vg, panelPoint(kSyncCenter.x, inputLabelY), "IN 1", 7.2F, primary);
        drawLabel(args.vg, panelPoint(kResetCenter.x, inputLabelY), "IN 2", 7.2F, primary);

        for (std::size_t index = 0U; index < kOutputCenters.size(); ++index) {
            const auto& center = kOutputCenters[index];
            const float labelY = center.y + kJackNutDiameterMm * 0.5F + 2.5F;
            const std::string label = std::to_string(index + 1U);
            drawLabel(args.vg, panelPoint(center.x, labelY), label.c_str(), 7.6F, primary);
        }

        drawLabel(args.vg, panelPoint(panelCenterX, 123.15F), "SOUTH SIGNAL LAB", 6.5F, primary);
    }
};

/**
 * @brief Endless push encoder with conflict-free Rack gestures.
 *
 * Press-and-drag vertically rotates the encoder in discrete detents, matching normal Rack knob
 * interaction while preserving the hardware's endless nature. A quick stationary click becomes a
 * short push on release; a stationary hold becomes a live push only after a grace period. Gesture
 * classification is exclusive for the full mouse-down interval, so rotation cannot also click.
 */
struct ClockEncoderWidget final : widget::OpaqueWidget {
    ClockModule* clockModule = nullptr;
    float visibleDiameterPx = 0.0F;
    float dragAccumulatorPx = 0.0F;
    float verticalTravelPx = 0.0F;
    float indicatorAngle = -1.57079632679489661923F;
    bool pointerDown = false;
    bool rotating = false;

    static constexpr float kPi = 3.14159265358979323846F;
    static constexpr float kDragThresholdPx = 2.0F;
    static constexpr float kPixelsPerDetent = 5.0F;
    static constexpr float kAnglePerDetent = 2.0F * kPi / 20.0F;

    void queueDetents(const int detents) {
        if (detents == 0 || clockModule == nullptr) {
            return;
        }
        clockModule->queueEncoderDetents(detents);
        indicatorAngle = std::remainder(
            indicatorAngle + static_cast<float>(detents) * kAnglePerDetent,
            2.0F * kPi);
    }

    void draw(const DrawArgs& args) override {
        const Vec center = box.size.div(2.0F);
        const float radius = std::max(2.0F, visibleDiameterPx * 0.5F);
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, center.x, center.y, radius);
        nvgFillColor(args.vg, nvgRGB(47, 48, 51));
        nvgFill(args.vg);
        nvgStrokeWidth(args.vg, 1.2F);
        nvgStrokeColor(args.vg, nvgRGB(18, 19, 21));
        nvgStroke(args.vg);

        nvgBeginPath(args.vg);
        nvgCircle(args.vg, center.x, center.y, std::max(1.0F, radius - 2.2F));
        nvgFillColor(args.vg, nvgRGB(37, 38, 40));
        nvgFill(args.vg);

        const float inner = radius * 0.28F;
        const float outer = radius * 0.78F;
        const float sx = center.x + std::cos(indicatorAngle) * inner;
        const float sy = center.y + std::sin(indicatorAngle) * inner;
        const float ex = center.x + std::cos(indicatorAngle) * outer;
        const float ey = center.y + std::sin(indicatorAngle) * outer;
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, sx, sy);
        nvgLineTo(args.vg, ex, ey);
        nvgStrokeWidth(args.vg, 1.5F);
        nvgStrokeColor(args.vg, nvgRGB(240, 241, 242));
        nvgLineCap(args.vg, NVG_ROUND);
        nvgStroke(args.vg);
    }

    void onButton(const ButtonEvent& event) override {
        if (event.button != GLFW_MOUSE_BUTTON_LEFT) {
            return;
        }
        if (event.action == GLFW_PRESS) {
            pointerDown = true;
            rotating = false;
            dragAccumulatorPx = 0.0F;
            verticalTravelPx = 0.0F;
            if (clockModule != nullptr) {
                clockModule->beginEncoderGesture();
            }
            event.consume(this);
        }
    }

    void onDragStart(const DragStartEvent& event) override {
        if (event.button == GLFW_MOUSE_BUTTON_LEFT) {
            APP->window->cursorLock();
        }
    }

    void onDragMove(const DragMoveEvent& event) override {
        if (event.button != GLFW_MOUSE_BUTTON_LEFT || !pointerDown) {
            return;
        }
        verticalTravelPx += std::abs(event.mouseDelta.y);
        if (!rotating && verticalTravelPx >= kDragThresholdPx && clockModule != nullptr) {
            rotating = clockModule->beginEncoderRotation();
        }
        if (rotating) {
            dragAccumulatorPx += -event.mouseDelta.y;
            const int detents = static_cast<int>(dragAccumulatorPx / kPixelsPerDetent);
            if (detents != 0) {
                dragAccumulatorPx -= static_cast<float>(detents) * kPixelsPerDetent;
                queueDetents(detents);
            }
        }
        event.consume(this);
    }

    void onDragEnd(const DragEndEvent& event) override {
        if (event.button != GLFW_MOUSE_BUTTON_LEFT) {
            return;
        }
        APP->window->cursorUnlock();
        pointerDown = false;
        if (clockModule != nullptr) {
            clockModule->endEncoderGesture();
        }
        rotating = false;
        dragAccumulatorPx = 0.0F;
        verticalTravelPx = 0.0F;
    }

    void onHoverScroll(const HoverScrollEvent& event) override {
        if (event.scrollDelta.y == 0.0F) {
            return;
        }
        queueDetents(event.scrollDelta.y > 0.0F ? 1 : -1);
        event.consume(this);
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

    ClockModule* clockModule() const {
        return dynamic_cast<ClockModule*>(module);
    }

    void onHoverKey(const HoverKeyEvent& event) override {
        ClockModule* const clock = clockModule();
        if (clock == nullptr) {
            ModuleWidget::onHoverKey(event);
            return;
        }

        const bool pressed = event.action == GLFW_PRESS || event.action == GLFW_REPEAT;
        const bool released = event.action == GLFW_RELEASE;
        const int nonShiftMods = event.mods & (GLFW_MOD_CONTROL | GLFW_MOD_ALT | GLFW_MOD_SUPER);

        // Shift is CLOCK's virtual second hand: while held, it is the physical TAP/SHIFT button.
        if (event.key == GLFW_KEY_LEFT_SHIFT || event.key == GLFW_KEY_RIGHT_SHIFT) {
            if (pressed || released) {
                clock->setKeyboardShiftHeld(event.key, pressed);
                event.consume(this);
            }
            return;
        }

        // Preserve Rack's own Ctrl/Alt/Super shortcuts. Shift remains available as a physical
        // TAP/SHIFT hold so Shift+Enter and Shift+Up/Down reproduce two-control hardware chords.
        if (nonShiftMods != 0) {
            ModuleWidget::onHoverKey(event);
            return;
        }

        if (event.key == GLFW_KEY_UP || event.key == GLFW_KEY_DOWN) {
            if (pressed) {
                clock->queueEncoderDetents(event.key == GLFW_KEY_UP ? 1 : -1);
            }
            event.consume(this);
            return;
        }

        if (event.key == GLFW_KEY_ENTER || event.key == GLFW_KEY_E) {
            if (pressed || released) {
                clock->setKeyboardEncoderHeld(pressed);
            }
            event.consume(this);
            return;
        }

        if (event.key == GLFW_KEY_P) {
            if (pressed || released) {
                clock->setKeyboardPlayHeld(pressed);
            }
            event.consume(this);
            return;
        }

        if (event.key == GLFW_KEY_T) {
            if (pressed || released) {
                clock->setKeyboardTapHeld(pressed);
            }
            event.consume(this);
            return;
        }

        if (event.key == GLFW_KEY_S || event.key == GLFW_KEY_BACKSPACE) {
            if (pressed || released) {
                clock->setKeyboardStopHeld(pressed);
            }
            event.consume(this);
            return;
        }

        ModuleWidget::onHoverKey(event);
    }

    void appendContextMenu(Menu* menu) override {
        ClockModule* const clock = clockModule();
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("South Signal Lab CLOCK"));
        menu->addChild(createMenuItem(
            "General Settings...",
            "",
            [clock]() {
                if (clock != nullptr) {
                    clock->requestGeneralSettings();
                }
            }));
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("VCV controls"));
        menu->addChild(createMenuLabel("Shift: hold TAP / SHIFT"));
        menu->addChild(createMenuLabel("Enter / E: encoder push"));
        menu->addChild(createMenuLabel("Up / Down: encoder turn"));
        menu->addChild(createMenuLabel("P: PLAY / PAUSE"));
        menu->addChild(createMenuLabel("T: TAP"));
        menu->addChild(createMenuLabel("S / Backspace: STOP / BACK"));
    }

    explicit ClockWidget(ClockModule* module) {
        using namespace clockfw::vcv::panel;

        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/CLOCK.svg")));

        auto* labels = createWidget<ClockPanelLabels>(Vec(0.0F, 0.0F));
        labels->box.size = box.size;
        addChild(labels);

        auto* display = createWidget<ClockDisplayWidget>(
            mm2px(Vec(kDisplay.x + kPanelOffsetXmm, kDisplay.y + kPanelOffsetYmm)));
        display->box.size = mm2px(Vec(kDisplay.width, kDisplay.height));
        display->module = module;
        addChild(display);

        // The physical encoder is an endless relative control, not a bounded Rack parameter.
        // One combined widget classifies each mouse-down as either vertical rotation or push and
        // keeps that classification until release, so the two gestures cannot overlap.
        constexpr float kEncoderHitDiameterMm = 12.0F;
        const Vec encoderCenter = panelPoint(kEncoderCenter);
        const Vec encoderSize = mm2px(Vec(kEncoderHitDiameterMm, kEncoderHitDiameterMm));
        auto* encoder = createWidget<ClockEncoderWidget>(encoderCenter.minus(encoderSize.div(2.0F)));
        encoder->box.size = encoderSize;
        encoder->visibleDiameterPx = mm2px(Vec(kEncoderDiameterMm, kEncoderDiameterMm)).x;
        encoder->clockModule = module;
        addChild(encoder);

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
