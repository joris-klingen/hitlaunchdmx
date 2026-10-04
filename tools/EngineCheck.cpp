// Headless check of the live engine: drives LiveEngine::tick with a synthetic
// clock (no GUI, no MIDI devices, ENTTEC not connected) and asserts that the
// Launchpad/XL state turns into the expected hitnotedmx output.
//
// Run: ctest --test-dir build_dev -C Release   (or the engine-check binary)

#include <cmath>
#include <cstdio>

#include "LiveEngine.h"

using namespace hitlaunchdmx;

namespace
{
int failures = 0;

void check (bool ok, const char* what)
{
    std::printf ("%s  %s\n", ok ? "ok  " : "FAIL", what);
    if (! ok) ++failures;
}

double now = 1000.0;

void run (LiveEngine& e, double seconds)
{
    const int n = static_cast<int> (seconds * 100.0);
    for (int i = 0; i < n; ++i)
    {
        now += 0.01;
        e.tick (now);
    }
}

// Sum of R, G, B over one bar of the default 4 × 18 rig.
struct Sum { float r = 0, g = 0, b = 0; };
Sum barSum (const LiveEngine& e, int bar)
{
    const Rig rig = e.getRig();
    Sum s;
    for (int px = 1; px <= rig.rows; ++px)
    {
        const auto ch = rig.channelsFor (bar, px);
        s.r += e.getDmxValues().get (ch[0]);
        s.g += e.getDmxValues().get (ch[1]);
        s.b += e.getDmxValues().get (ch[2]);
    }
    return s;
}
}  // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;   // HighResolutionTimer / MessageManager plumbing
    EnttecProDmx dmx;                            // never connected
    LiveEngine e (dmx);
    e.tick (now);

    // ── Launchpad builder: Chase + Red ──
    e.setViewPage (kPageMotion);
    e.padPressed (8, 1);                                   // Chases, first pad = Chase
    check (e.getSel (hitnotedmx::kChasesStart) != 0, "motion pad (8,1) latches Chase (24)");
    e.setViewPage (kPageColour);
    e.padPressed (8, 2);                                   // Primary #1 = Red (85)
    check (e.getSel (hitnotedmx::kPrimaryPaletteStart + 1) != 0, "colour pad (8,2) latches Red (85)");

    run (e, 2.0);
    float red = 0, other = 0;
    for (int b = 0; b < 4; ++b) { const auto s = barSum (e, b); red += s.r; other += s.g + s.b; }
    check (red > 0.5f, "red chase lights the bars");
    check (other < 1.0e-4f, "red chase emits no green/blue");

    // ── group exclusivity / Shift ──
    e.padPressed (8, 3);                                   // Orange-red replaces Red
    check (e.getSel (85) == 0 && e.getSel (86) != 0, "new primary replaces the old one");
    e.topButton (TopButton::Shift, true);
    e.padPressed (8, 2);
    e.topButton (TopButton::Shift, false);
    check (e.getSel (85) != 0 && e.getSel (86) != 0, "Shift adds to the group");
    e.padPressed (8, 3);
    check (e.getSel (86) == 0, "tapping a lit pad unlatches it");

    // ── zones cycle primary → secondary → off ──
    e.setViewPage (kPageMotion);
    e.padPressed (2, 1);
    check (e.getSel (kZonePitch0) >= 64, "zone tap 1 = primary route");
    e.padPressed (2, 1);
    check (e.getSel (kZonePitch0) > 0 && e.getSel (kZonePitch0) < 64, "zone tap 2 = secondary route");
    e.padPressed (2, 1);
    check (e.getSel (kZonePitch0) == 0, "zone tap 3 = off");

    // ── bar fader ──
    e.setParam (kFaderParam0 + 0, 0.0f);
    run (e, 0.5);
    {
        const auto s0 = barSum (e, 0);
        check (s0.r + s0.g + s0.b < 1.0e-4f, "bar 1 fader at 0 darkens bar 1");
    }
    e.setParam (kFaderParam0 + 0, 1.0f);

    // ── clock: 120 BPM, relative speed switches only on a global beat ──
    e.restartClock();
    run (e, 0.01);
    run (e, 2.0);
    check (std::abs (e.getGlobalBeats() - 4.0) < 0.05, "120 BPM: 2 s = 4 beats");
    run (e, 0.15);                                         // now ~beat 4.3
    e.setParam (idx (P::RelSpeed), 1.0f);                  // 4×
    run (e, 0.1);                                          // ~beat 4.5
    check (std::abs (e.getAppliedRelSpeed() - 1.0) < 1e-9, "rel speed waits for the next beat");
    run (e, 0.3);                                          // past beat 5
    check (std::abs (e.getAppliedRelSpeed() - 4.0) < 1e-9, "rel speed applied on the beat");
    e.setParam (idx (P::RelSpeed), 0.5f);

    // ── tap tempo ──
    for (int i = 0; i < 4; ++i)
    {
        // tap() reads the real clock, so space the taps in real time.
        e.tap();
        juce::Thread::sleep (500);
    }
    check (std::abs (e.getBpm() - 120.0) < 6.0, "4 taps 0.5 s apart ≈ 120 BPM");

    // ── blackout ──
    e.buttonEvent (idx (B::Blackout), true);
    e.buttonEvent (idx (B::Blackout), false);
    run (e, 0.2);
    {
        float sum = 0;
        for (int ch = 1; ch <= e.getRig().rigChannels(); ++ch) sum += e.getDmxValues().get (ch);
        check (sum < 1.0e-4f, "Blackout toggle kills the rig");
    }
    e.buttonEvent (idx (B::Blackout), true);
    e.buttonEvent (idx (B::Blackout), false);
    check (! e.isButtonOn (idx (B::Blackout)), "Blackout toggles back off");

    // ── looks: store + recall ──
    e.setViewPage (kFirstLookPage);
    e.topButton (TopButton::Store, true);
    e.topButton (TopButton::Store, false);
    e.padPressed (8, 1);
    check (e.lookUsed (0) && e.getActiveLook() == 0, "Store + pad saves look 1");
    e.clearSelection();
    check (e.getSel (85) == 0, "Clear empties the selection");
    e.padPressed (8, 1);
    check (e.getSel (85) != 0 && e.getSel (hitnotedmx::kChasesStart) != 0, "look pad recalls the stored look");

    // ── persistence round-trip ──
    {
        auto xml = e.toXml();
        LiveEngine e2 (dmx);
        e2.fromXml (*xml);
        check (e2.lookUsed (0) && e2.getSel (85) != 0, "state survives toXml/fromXml");
    }

    std::printf (failures == 0 ? "\nALL OK\n" : "\n%d FAILURE(S)\n", failures);
    return failures == 0 ? 0 : 1;
}
