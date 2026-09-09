/**
 * @file beatknecht.cpp
 * @brief Eight-channel gate rhythm generator Easter egg implementation.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "game/beatknecht.h"

#include <algorithm>
#include <array>
#include <cstdio>

#include "hal/system_clock.h"
#include "ui_text.h"

namespace clockfw::game {
namespace {
constexpr std::uint32_t kFrameIntervalMs = 45U;
constexpr std::uint32_t kGatePulseMs = 12U;
constexpr std::uint32_t kExitLongPressMs = 900U;
constexpr std::uint16_t kMinimumBpm = 40U;
constexpr std::uint16_t kMaximumBpm = 240U;

struct DrumStyle final {
    text::TextId name;
    std::uint16_t defaultBpm;
    std::array<std::uint16_t, 8U> pattern;
};

// Curated one-bar 16th-note templates. They are intentionally generic genre
// starting points rather than transcriptions of any protected recording.
constexpr std::array<DrumStyle, 14U> kStyles{{
    {text::TextId::DrummerEdm,      128U, {{0x1111,0x1010,0x1010,0x4444,0x4040,0x0000,0x2222,0x8000}}},
    {text::TextId::DrummerHouse,    124U, {{0x1111,0x1010,0x1010,0x4444,0x4444,0x0000,0x2222,0x8888}}},
    {text::TextId::DrummerTechno,   132U, {{0x1111,0x1010,0x0000,0x5555,0x2222,0x0800,0x8421,0x8080}}},
    {text::TextId::DrummerHipHop,    92U, {{0x0489,0x1010,0x1000,0x5555,0x0400,0x0200,0x2202,0x8000}}},
    {text::TextId::DrummerTrap,     142U, {{0x1289,0x1010,0x1000,0xFFFF,0x4080,0x0200,0x0A20,0x8000}}},
    {text::TextId::DrummerPop,      112U, {{0x0911,0x1010,0x1010,0x5555,0x4040,0x0800,0x0202,0x8000}}},
    {text::TextId::DrummerRock,     118U, {{0x0511,0x1010,0x0000,0x5555,0x4444,0x0800,0x0002,0x8000}}},
    {text::TextId::DrummerBlues,     92U, {{0x0249,0x1040,0x0000,0x9249,0x0000,0x2080,0x0492,0x8000}}},
    {text::TextId::DrummerFunk,     104U, {{0x2291,0x1010,0x0808,0xD555,0x2020,0x0804,0x8422,0x8000}}},
    {text::TextId::DrummerReggae,    78U, {{0x0100,0x1010,0x0000,0x4444,0x0404,0x0000,0x2222,0x8000}}},
    {text::TextId::DrummerSalsa,    104U, {{0x0909,0x2020,0x4104,0xAAAA,0x0000,0x2492,0x9249,0x8080}}},
    {text::TextId::DrummerSamba,    108U, {{0x5151,0x1010,0x0440,0xAAAA,0x0000,0x2492,0xDB6D,0x8080}}},
    {text::TextId::DrummerBossaNova,128U, {{0x1111,0x1010,0x4104,0xAAAA,0x0000,0x2492,0x8421,0x8000}}},
    {text::TextId::DrummerDnb,      172U, {{0x0149,0x1010,0x0800,0xFFFF,0x4444,0x0080,0x2222,0x8000}}}
}};
constexpr std::array<char,8U> kRowLabels{{'K','S','C','H','O','T','P','X'}};
}  // namespace

Beatknecht::Beatknecht(hal::OledDisplay& display, hal::ControlPanel& controls, hal::GateOutputDriver& gateOutputs)
    : display_(display), controls_(controls), gateOutputs_(gateOutputs), shell_(display, ArcadeTitle::Beatknecht, nullptr) {}

void Beatknecht::run() {
    gateOutputs_.disableOutputStage(); gateOutputs_.setAllChannelsLow();
    const std::uint32_t startedAtMs = hal::SystemClock::milliseconds(); shell_.begin(startedAtMs);
#ifdef CLOCK_HOST_TEST
    shell_.startImmediatelyForTest(); resetSession(startedAtMs); render(); return;
#else
    bool outputsEnabled = false;
    std::uint32_t lastFrameAtMs = startedAtMs;
    while (!exitRequested_) {
        const std::uint32_t nowMs = hal::SystemClock::milliseconds();
        const hal::ControlSample controls = controls_.sample(nowMs);
        if (controls.encoderButton.edge == hal::ButtonEdge::Pressed) encoderPressedAtMs_ = nowMs;
        if (!controls.encoderButton.pressed) encoderPressedAtMs_ = 0U;
        else if (encoderPressedAtMs_ != 0U && nowMs - encoderPressedAtMs_ >= kExitLongPressMs) exitRequested_ = true;
        const ArcadeShell::Action action = shell_.update(controls, nowMs);
        if (action == ArcadeShell::Action::StartRun || action == ArcadeShell::Action::RestartRun) {
            resetSession(nowMs); gateOutputs_.enableOutputStage(); outputsEnabled = true;
        }
        if (shell_.playing()) update(controls, nowMs);
        if (nowMs - lastFrameAtMs >= kFrameIntervalMs) { if (shell_.playing()) render(); else shell_.render(nowMs); lastFrameAtMs = nowMs; }
        (void)display_.service();
        hal::SystemClock::delayMilliseconds(1U);
    }
    if (outputsEnabled) stopOutputs(); gateOutputs_.disableOutputStage();
#endif
}

#ifdef CLOCK_SIMULATOR
void Beatknecht::beginForSimulator() {
    gateOutputs_.disableOutputStage(); gateOutputs_.setAllChannelsLow();
    const std::uint32_t nowMs = hal::SystemClock::milliseconds(); shell_.begin(nowMs);
    lastSimulatorFrameAtMs_ = nowMs; shell_.render(nowMs);
}

bool Beatknecht::serviceForSimulator(const std::uint32_t nowMs) {
    const hal::ControlSample controls = controls_.sample(nowMs);
    if (controls.encoderButton.edge == hal::ButtonEdge::Pressed) encoderPressedAtMs_ = nowMs;
    if (!controls.encoderButton.pressed) encoderPressedAtMs_ = 0U;
    else if (encoderPressedAtMs_ != 0U && nowMs - encoderPressedAtMs_ >= kExitLongPressMs) exitRequested_ = true;
    const ArcadeShell::Action action = shell_.update(controls, nowMs);
    if (action == ArcadeShell::Action::StartRun || action == ArcadeShell::Action::RestartRun) { resetSession(nowMs); gateOutputs_.enableOutputStage(); }
    if (shell_.playing()) update(controls, nowMs);
    if (nowMs - lastSimulatorFrameAtMs_ >= kFrameIntervalMs) { if (shell_.playing()) render(); else shell_.render(nowMs); lastSimulatorFrameAtMs_ = nowMs; }
    if (exitRequested_) { stopOutputs(); gateOutputs_.disableOutputStage(); }
    return !exitRequested_;
}
#endif

void Beatknecht::resetSession(const std::uint32_t nowMs) {
    styleIndex_ = 0U; bpm_ = kStyles[0].defaultBpm; nextStep_ = 0U; displayStep_ = 0U;
    nextStepAtMs_ = nowMs; gatesOffAtMs_ = 0U; encoderPressedAtMs_ = 0U; gatesHigh_ = false; exitRequested_ = false;
}

std::uint32_t Beatknecht::stepDurationMs() const {
    return 15000U / static_cast<std::uint32_t>(bpm_); // quarter note / 4 = 16th note
}

void Beatknecht::stopOutputs() {
    gateOutputs_.setAllChannelsLow(); gatesHigh_ = false;
}

void Beatknecht::triggerStep(const std::uint32_t nowMs) {
    stopOutputs();
    const DrumStyle& style = kStyles[styleIndex_];
    displayStep_ = nextStep_;
    const std::uint16_t bit = static_cast<std::uint16_t>(1U << nextStep_);
    bool any = false;
    for (std::size_t channel = 0U; channel < style.pattern.size(); ++channel) {
        const bool high = (style.pattern[channel] & bit) != 0U;
        gateOutputs_.setChannelState(channel, high); any = any || high;
    }
    gatesHigh_ = any; gatesOffAtMs_ = nowMs + kGatePulseMs;
    nextStep_ = static_cast<std::uint8_t>((nextStep_ + 1U) & 0x0FU);
    nextStepAtMs_ = nowMs + stepDurationMs();
}

void Beatknecht::update(const hal::ControlSample& controls, const std::uint32_t nowMs) {
    if (controls.tapButton.edge == hal::ButtonEdge::Pressed) {
        styleIndex_ = static_cast<std::uint8_t>((styleIndex_ + 1U) % kStyles.size());
    }
    if (controls.encoderDelta != 0) {
        bpm_ = static_cast<std::uint16_t>(std::clamp<int>(static_cast<int>(bpm_) + controls.encoderDelta, kMinimumBpm, kMaximumBpm));
    }
    if (gatesHigh_ && nowMs >= gatesOffAtMs_) stopOutputs();
    if (nowMs >= nextStepAtMs_) triggerStep(nowMs);
}

void Beatknecht::render() {
    display_.clear(); display_.setFont(hal::DisplayFont::Small); display_.setTextColor(hal::PixelColor::White);
    const DrumStyle& style = kStyles[styleIndex_];
    display_.drawText(1, 0, text::get(style.name));
    char tempoText[10]{}; std::snprintf(tempoText, sizeof(tempoText), "%u", static_cast<unsigned>(bpm_));
    const hal::TextBounds bounds = display_.measureText(tempoText, 0, 0);
    display_.drawText(static_cast<std::int16_t>(127 - bounds.width), 0, tempoText);
    for (std::uint8_t row = 0U; row < 8U; ++row) {
        const std::int16_t y = static_cast<std::int16_t>(8 + row * 7U);
        display_.drawCharacter(1, y, kRowLabels[row]);
        for (std::uint8_t step = 0U; step < 16U; ++step) {
            const std::int16_t x = static_cast<std::int16_t>(24 + step * 6U);
            const bool hit = (style.pattern[row] & static_cast<std::uint16_t>(1U << step)) != 0U;
            if (hit) display_.fillRectangle(x, static_cast<std::int16_t>(y + 1), 4, 4);
            else display_.setPixel(static_cast<std::int16_t>(x + 1), static_cast<std::int16_t>(y + 3));
            if (step == displayStep_) display_.drawRectangle(static_cast<std::int16_t>(x - 1), y, 6, 6);
        }
    }
    display_.present();
}

}  // namespace clockfw::game
