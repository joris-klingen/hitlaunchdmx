#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_graphics/juce_graphics.h>

#include "LiveEngine.h"

namespace hitlaunchdmx
{

// Small thread-safe text log of incoming surface MIDI (GUI diagnostics only —
// several MIDI threads write it, so it's locked, never on the engine tick).
class ActivityLog
{
public:
    void add (const juce::String& line);
    bool takeIfChanged (juce::String& out);   // message thread

private:
    juce::CriticalSection lock;
    juce::StringArray     lines;
    bool                  dirty = false;
};

// ── What a pad / button shows — shared by hardware LEDs and the on-screen ──
// ── surfaces, so the screen is always an exact mirror of the hardware.    ──

struct PadView
{
    juce::Colour colour;     // what the LED shows (already dimmed if idle)
    juce::String label;      // on-screen caption
    bool         lit = false;
};

// Launchpad: row 1..8 = grid (1 = bottom), row 9 = top round buttons;
// col 1..8 = grid, col 9 = right-hand page buttons. (9, 9) is the logo.
PadView describeLaunchpad (const LiveEngine&, int row, int col);

// XL: LED colour + state for a strip button / knob, as an on-screen colour.
PadView describeXlButton (const LiveEngine&, int b);

// ── Launchpad Mini MK3 (Programmer mode) ──
class LaunchpadSurface : private juce::MidiInputCallback
{
public:
    LaunchpadSurface (LiveEngine&, ActivityLog&);
    ~LaunchpadSurface() override;

    bool open (const juce::String& inName, const juce::String& outName);
    void close();
    bool isOpen() const noexcept { return midiIn != nullptr; }
    juce::String getName() const { return openName; }

    void refreshLeds();   // message thread; sends one RGB SysEx for all LEDs

    static juce::String findInput();    // best-guess device names (or empty)
    static juce::String findOutput();

private:
    void handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage&) override;

    LiveEngine&  engine;
    ActivityLog& log;
    std::unique_ptr<juce::MidiInput>  midiIn;
    std::unique_ptr<juce::MidiOutput> midiOut;
    juce::String openName;
};

// ── Launch Control XL (Factory template 1) ──
class LaunchControlXlSurface : private juce::MidiInputCallback
{
public:
    LaunchControlXlSurface (LiveEngine&, ActivityLog&);
    ~LaunchControlXlSurface() override;

    bool open (const juce::String& inName, const juce::String& outName);
    void close();
    bool isOpen() const noexcept { return midiIn != nullptr; }
    juce::String getName() const { return openName; }

    void refreshLeds();   // message thread

    static juce::String findInput();
    static juce::String findOutput();

private:
    void handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage&) override;

    LiveEngine&  engine;
    ActivityLog& log;
    std::unique_ptr<juce::MidiInput>  midiIn;
    std::unique_ptr<juce::MidiOutput> midiOut;
    juce::String openName;
};

}  // namespace hitlaunchdmx
