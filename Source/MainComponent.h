#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "DmxVisualizer.h"
#include "EnttecProDmx.h"
#include "MidiLog.h"
#include "SceneEngine.h"

namespace hitlaunchdmx
{

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
    EnttecProDmx dmx;
    DmxFrame     liveFrame;
    MidiLog      midiLog;
    SceneEngine  engine;

    // --- visualiser ---
    DmxVisualizer visualizer;

    // --- left panel controls ---
    juce::Label       midiInLabel    { {}, "MIDI In:" };
    juce::ComboBox    midiInCombo;
    juce::Label       midiOutLabel   { {}, "MIDI Out:" };
    juce::ComboBox    midiOutCombo;
    juce::TextButton  connectBtn     { "Connect USB" };
    juce::TextButton  disconnectBtn  { "Disconnect" };
    juce::TextButton  blackoutBtn    { "Blackout" };
    juce::TextButton  lpModeBtn      { "LP Prog Mode" };
    juce::Label       dmxStatusLabel { {}, "" };
    juce::TextEditor  midiLogEditor;

    // --- right panel ---
    juce::Label  sceneNameLabel  { {}, "Scene:" };
    juce::Label  sceneValueLabel { {}, "—" };
    juce::Label  xfadeLabel      { {}, "Crossfade (s):" };
    juce::Slider xfadeSlider;

    bool blackoutActive { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

}  // namespace hitlaunchdmx
