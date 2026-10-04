#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "DmxVisualizer.h"   // hitnotedmx
#include "EnttecProDmx.h"    // hitnotedmx
#include "LiveEngine.h"
#include "Surfaces.h"

namespace hitlaunchdmx
{

// Custom rotary look (adapted from hitnotedmx): dark body, coloured value arc
// from rotarySliderFillColourId, white pointer.
class DimKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;
};

// On-screen Launchpad Mini MK3: the 8×8 grid + top round buttons + right-hand
// page buttons, drawn from describeLaunchpad() — an exact mirror of the
// hardware LEDs, and clickable (shift-click = Shift held).
class VirtualLaunchpad : public juce::Component
{
public:
    explicit VirtualLaunchpad (LiveEngine& e) : engine (e) {}

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> cellBounds (int row, int col) const;   // row/col 1..9
    std::pair<int, int>    cellAt (juce::Point<float>) const;

    LiveEngine& engine;
    int  pressedTop = -1;         // top button held by the mouse (for release)
    bool mouseShift = false;
};

// On-screen Launch Control XL: shows every control's mapping (name + live
// value) and state, and can be operated with the mouse.
class XlPanel : public juce::Component
{
public:
    XlPanel (LiveEngine&, juce::LookAndFeel&);
    ~XlPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void sync();   // pull engine state (GUI timer)

private:
    struct XlButton : public juce::Component
    {
        XlButton (LiveEngine& e, int i) : engine (e), index (i) {}
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override { engine.buttonEvent (index, true); }
        void mouseUp   (const juce::MouseEvent&) override { engine.buttonEvent (index, false); }
        LiveEngine& engine;
        int index;
    };

    LiveEngine& engine;
    std::array<juce::Slider, kNumParams> controls;
    std::array<juce::Label,  kNumParams> names;
    std::array<juce::Label,  kNumParams> values;
    std::vector<std::unique_ptr<XlButton>> buttons;
    juce::Rectangle<int> stripsArea;
};

class MainComponent : public juce::Component,
                      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void rescanDevices();
    void openSurfacesFromCombos();
    void loadState();
    void saveState();
    static juce::File stateFile();

    hitnotedmx::EnttecProDmx dmx;
    LiveEngine               engine { dmx };
    ActivityLog              activity;
    LaunchpadSurface         launchpad { engine, activity };
    LaunchControlXlSurface   xlSurface { engine, activity };

    DimKnobLookAndFeel       knobLnf;
    hitnotedmx::DmxVisualizer visualizer { engine.getDmxValues(), engine.getSelection() };
    VirtualLaunchpad         virtualLaunchpad { engine };
    XlPanel                  xlPanel { engine, knobLnf };

    // left panel
    juce::TextButton dmxButton    { "Connect DMX" };
    juce::Label      dmxStatus;
    juce::Label      lpLabel      { {}, "Launchpad in / out" };
    juce::ComboBox   lpIn, lpOut;
    juce::Label      xlLabel      { {}, "Launch Control XL in / out" };
    juce::ComboBox   xlIn, xlOut;
    juce::TextButton rescanButton { "Rescan MIDI" };
    juce::Label      gridLabel    { {}, "Grid (bars x pixels)" };
    juce::ComboBox   gridCols, gridRows;
    juce::TextButton gridSet      { "Set grid" };
    juce::Label      tempoLabel;
    juce::TextEditor logView;

    std::uint32_t lastSerial = 0xffffffff;
    std::uint32_t lastPadSerial = 0xffffffff;
    bool          lastTapBlink = false;
    std::uint32_t savedSerial = 0;
    int           saveCountdown = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

}  // namespace hitlaunchdmx
