#pragma once

#include <array>
#include <atomic>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

#include "Composition.h"   // hitnotedmx: computeDmx, GridState, DmxValues, …
#include "Controls.h"
#include "EnttecProDmx.h"  // hitnotedmx
#include "MidiState.h"     // hitnotedmx
#include "PadLayout.h"

namespace hitlaunchdmx
{

using hitnotedmx::DmxValues;
using hitnotedmx::EnttecProDmx;
using hitnotedmx::Rig;
using hitnotedmx::SelectionMask;

// A stored look: the latched note set (per pitch: 0 = off, 127 = held; the
// engine replaces the velocity with the XL knob for that bank).
struct Look
{
    bool used = false;
    std::array<std::uint8_t, 128> sel {};
};

// The live engine — hitlaunchdmx's whole runtime, minus the surfaces/GUI.
//
// It does NOT render anything itself. Each tick (100 Hz, own thread) it turns
// the current state into the set of hitnotedmx notes that state means — the
// Launchpad-latched look + the XL's buttons and knob values as velocities —
// and hands that MidiState to hitnotedmx's computeDmx, exactly as hitnotedmx's
// processBlock does with notes from the DAW. Same grid, same recipes, same
// masters; this app is only a different way of holding the notes.
//
// Timing: there is no host. A global beat clock runs at the BPM knob (or the
// tapped tempo); the clock handed to computeDmx is that clock × the relative
// speed (¼ … 4×). A relative-speed change is applied on the next global beat
// and the scaled clock is integrated (never jumps), so motion stays continuous
// and locked to the global beat. hitnotedmx's own Speed note (127) is never
// held — the relative knob replaces it. Beat-based lengths (bump release,
// crossfade) scale with the relative speed too, by design.
//
// Threading: every input (surfaces' MIDI threads, GUI) writes atomics or takes
// `stateLock` (never held by the tick). The tick only reads atomics and owns
// all hitnotedmx state. `stateSerial` bumps on every state change so the GUI
// timer knows when to refresh LEDs / virtual surfaces.
class LiveEngine : private juce::HighResolutionTimer
{
public:
    explicit LiveEngine (EnttecProDmx& dmx);
    ~LiveEngine() override;

    void start();
    void stop();

    // ── XL: params + buttons ──
    void  setParam (int p, float norm) noexcept;
    float getParam (int p) const noexcept { return params[static_cast<size_t> (p)].load (std::memory_order_relaxed); }
    void  buttonEvent (int b, bool down) noexcept;     // hardware or on-screen
    bool  isButtonOn (int b) const noexcept { return buttonOn[static_cast<size_t> (b)].load (std::memory_order_relaxed); }

    // ── Launchpad ──
    void padPressed (int row, int col) noexcept;        // on the current view page
    void topButton (TopButton b, bool down) noexcept;
    void setViewPage (int page) noexcept;
    int  getViewPage() const noexcept { return viewPage.load(); }
    bool isShiftHeld() const noexcept { return shiftHeld.load(); }
    bool isStoreArmed() const noexcept { return storeArmed.load(); }
    int  getActiveLook() const noexcept { return activeLook.load(); }

    // Latched selection (0 = off, else velocity hint).
    std::uint8_t getSel (int pitch) const noexcept { return sel[static_cast<size_t> (pitch)].load (std::memory_order_relaxed); }
    bool lookUsed (int slot) const;
    void clearSelection() noexcept;

    // ── Tempo ──
    void   tap() noexcept;
    void   restartClock() noexcept { restartRequest.store (true); }
    double getBpm() const noexcept { return bpmFromNorm (getParam (idx (P::Bpm))); }
    double getGlobalBeats() const noexcept { return publishedBeat.load (std::memory_order_relaxed); }
    double getAppliedRelSpeed() const noexcept { return appliedMult.load (std::memory_order_relaxed); }

    // ── Grid ──
    Rig  setGridShape (int cols, int rows);      // clamps; returns the applied shape
    Rig  getRig() const noexcept
    {
        const auto r = gridRequest.load (std::memory_order_relaxed);
        return { static_cast<int> (r >> 8), static_cast<int> (r & 0xffu) };
    }

    // ── Output for the GUI ──
    const DmxValues&     getDmxValues() const noexcept { return publishedValues; }
    const SelectionMask& getSelection() const noexcept { return publishedSelection; }
    float getStrobeHz() const noexcept { return strobeHzOut.load (std::memory_order_relaxed); }

    std::uint32_t getStateSerial() const noexcept { return stateSerial.load (std::memory_order_relaxed); }
    // Bumps only on changes the Launchpad shows (selection, page, looks,
    // Shift/Store, Blackout) — knob moves don't re-send the pad LEDs.
    std::uint32_t getPadSerial() const noexcept   { return padSerial.load (std::memory_order_relaxed); }

    // One engine step at wall-clock `nowSec` — the timer calls this; the
    // headless engine-check drives it directly with a synthetic clock.
    void tick (double nowSec);

    // ── Persistence (message thread) ──
    std::unique_ptr<juce::XmlElement> toXml() const;
    void fromXml (const juce::XmlElement&);

private:
    void hiResTimerCallback() override;
    void changed (bool affectsPads = true) noexcept
    {
        stateSerial.fetch_add (1, std::memory_order_relaxed);
        if (affectsPads) padSerial.fetch_add (1, std::memory_order_relaxed);
    }
    void latchPitch (int pitch) noexcept;        // holds stateLock
    void padOnLookPage (int slot) noexcept;      // holds stateLock

    EnttecProDmx& dmx;

    // ── inputs (any thread) ──
    std::array<std::atomic<float>, kNumParams>   params;
    std::array<std::atomic<bool>, kNumButtons>   buttonOn;
    std::array<std::atomic<std::uint32_t>, kNumButtons> buttonPresses;
    std::array<std::atomic<std::uint8_t>, 128>   sel;
    std::atomic<int>  viewPage   { kPageMotion };
    std::atomic<bool> shiftHeld  { false };
    std::atomic<bool> storeArmed { false };
    std::atomic<int>  activeLook { -1 };
    std::atomic<std::uint32_t> stateSerial { 0 };
    std::atomic<std::uint32_t> padSerial   { 0 };

    juce::SpinLock         stateLock;            // selection/looks read-modify-write
    std::array<Look, kNumLooks> looks;           // guarded by stateLock

    // tap tempo (guarded by stateLock)
    double lastTapSec = 0.0;
    std::array<double, 4> tapIntervals {};
    int    numTapIntervals = 0;
    std::atomic<bool> nudgeRequest   { false };  // align the beat phase to the tap
    std::atomic<bool> restartRequest { false };

    static constexpr std::uint32_t packGrid (int c, int r) noexcept
    {
        return (static_cast<std::uint32_t> (c) << 8) | static_cast<std::uint32_t> (r);
    }
    std::atomic<std::uint32_t> gridRequest { packGrid (hitnotedmx::kDefaultBars, hitnotedmx::kDefaultRows) };

    // ── tick-owned (engine thread) ──
    std::uint32_t appliedGrid = 0;
    hitnotedmx::GridState      grid;
    hitnotedmx::MidiState      midiState;
    hitnotedmx::ColorFadeState colorFade;
    hitnotedmx::BumpState      bumpState;
    DmxValues                  dmxValues;
    SelectionMask              selection;
    std::array<bool, 128>          wasOn {};
    std::array<double, 128>        startBeat {};
    std::array<std::uint32_t, kNumButtons> seenPresses {};

    double lastTickSec  = 0.0;
    double globalBeats  = 0.0;   // the master beat clock
    double scaledBeats  = 0.0;   // global × relative speed, integrated; fed to computeDmx
    double curMult      = 1.0;
    int    curStep      = 2;     // relative-speed step of curMult (2 = 1×)

    // ── published for the GUI ──
    DmxValues     publishedValues;
    SelectionMask publishedSelection;
    std::atomic<float>  strobeHzOut   { 0.0f };
    std::atomic<double> publishedBeat { 0.0 };
    std::atomic<double> appliedMult   { 1.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LiveEngine)
};

}  // namespace hitlaunchdmx
