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

    void setActiveScene(int idx) { activeScene = idx; repaint(); }

private:
    // Visual row 0 = top = LP row 8 = scenes 56–63 (wild)
    // scene index = (7 - visualRow) * 8 + visualCol
    static constexpr int kRows = 8;
    static constexpr int kCols = 8;

    static int sceneForCell(int vRow, int vCol) noexcept
    {
        return (kRows - 1 - vRow) * kCols + vCol;
    }

    juce::Rectangle<float> cellBoundsF(int vRow, int vCol) const;
    int cellForPoint(juce::Point<int> pt) const;

    SceneEngine& engine;
    int activeScene  { 0 };
    int hoveredScene { -1 };

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
    VirtualLaunchpad virtualLaunchpad;
    juce::Label      xfadeLabel      { {}, "Crossfade (s):" };
    juce::Slider     xfadeSlider;

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

    bool blackoutActive { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

}  // namespace hitlaunchdmx
