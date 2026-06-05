#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "DmxVisualizer.h"
#include "EnttecProDmx.h"
#include "MidiLog.h"
#include "Scene.h"
#include "SceneEngine.h"

namespace hitlaunchdmx
{

// ── DimKnobLookAndFeel ───────────────────────────────────────────────────────
// Custom rotary look (adapted from hitnotedmx): dark body, coloured value arc
// from rotarySliderFillColourId, white pointer. Each knob can be tinted.
class DimKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider&) override;
};

// ── VirtualLaunchpad ─────────────────────────────────────────────────────────
// Clickable 8×8 grid mirroring the Launchpad Mini MK3 pad layout.
// Visual row 0 (top)    = LP row 8 (notes 81–88) = wild chases 9–16
// Visual row 7 (bottom) = LP row 1 (notes 11–18) = breathes 1–8
class VirtualLaunchpad : public juce::Component
{
public:
    explicit VirtualLaunchpad(SceneEngine& eng);

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;

    // activeScene is a FLAT index (0..kTotalScenes-1).
    void setActiveScene(int flat) { if (flat != activeScene) { activeScene = flat; repaint(); } }
    void setViewPage(int page)    { if (page != viewPage)    { viewPage = page;   repaint(); } }

private:
    // Visual row 0 = top = LP row 8; row 7 = bottom = LP row 1.
    // in-page scene index = (7 - visualRow) * 8 + visualCol
    static constexpr int kRows = 8;
    static constexpr int kCols = 8;

    static int inPageForCell(int vRow, int vCol) noexcept
    {
        return (kRows - 1 - vRow) * kCols + vCol;
    }
    int flatForCell(int vRow, int vCol) const noexcept
    {
        return viewPage * kScenesPerPage + inPageForCell(vRow, vCol);
    }

    juce::Rectangle<float> cellBoundsF(int vRow, int vCol) const;
    int cellForPoint(juce::Point<int> pt) const;  // returns FLAT index

    SceneEngine& engine;
    int viewPage     { 0 };
    int activeScene  { 0 };   // flat
    int hoveredScene { -1 };  // flat

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VirtualLaunchpad)
};

// ── MainComponent ─────────────────────────────────────────────────────────────
class MainComponent : public juce::Component,
                      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void buildMidiDeviceMenus();
    void connectDmx();
    void disconnectDmx();

    // --- engine objects ---
    EnttecProDmx    dmx;
    DmxFrame        liveFrame;
    MidiLog         midiLog;
    SceneEngine     engine;

    // --- centre ---
    DmxVisualizer   visualizer;

    // --- right ---
    std::array<juce::TextButton, kNumPages> pageTabs;
    VirtualLaunchpad virtualLaunchpad;

    // --- left panel ---
    juce::Label      midiInLabel     { {}, "MIDI In:" };
    juce::ComboBox   midiInCombo;
    juce::Label      midiOutLabel    { {}, "MIDI Out:" };
    juce::ComboBox   midiOutCombo;
    juce::TextButton connectBtn      { "Connect USB" };
    juce::TextButton disconnectBtn   { "Disconnect" };
    juce::TextButton blackoutBtn     { "Blackout" };
    juce::TextButton lpModeBtn       { "LP Prog Mode" };
    juce::Label      dmxStatusLabel  { {}, "" };
    juce::TextEditor midiLogEditor;

    // --- four performance pots (left panel bottom) ---
    DimKnobLookAndFeel knobLnf;
    juce::Slider dimSlider   { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
    juce::Slider tailSlider  { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
    juce::Slider speedSlider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
    juce::Slider xfadeSlider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
    juce::Label  dimLabel    { {}, "DIM" };
    juce::Label  tailLabel   { {}, "TAIL" };
    juce::Label  speedLabel  { {}, "SPEED" };
    juce::Label  xfadeLabel  { {}, "XFADE" };

    bool blackoutActive { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

}  // namespace hitlaunchdmx
