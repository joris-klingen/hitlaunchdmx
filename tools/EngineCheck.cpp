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

    // ── Motion page: colour column (cols 7-8, rows 8-3) + Multicolor rows 2-1 ──
    e.setViewPage (kPageMotion);
    check (padPitch (kPageMotion, 8, 7) == 85 && padPitch (kPageMotion, 8, 8) == 87,
           "Motion row 8 cols 7-8 = Red, Orange");
    check (padPitch (kPageMotion, 3, 8) == hitnotedmx::kPrimaryPaletteStart + 22, "Motion (3,8) = Cool white");
    check (padPitch (kPageMotion, 2, 1) == hitnotedmx::kColorDynStart && padPitch (kPageMotion, 1, 8) == hitnotedmx::kColorDynStart + 15,
           "Motion rows 2-1 = Multicolor Rainbow .. Night sky");
    check (padPitch (kPageMotion, 3, 6) == hitnotedmx::kWildStart + 11, "Motion (3,6) = last Wild (Burst)");
    bool noZones = true;
    for (int row = 1; row <= 8; ++row)
        for (int col = 1; col <= 8; ++col)
            for (int pg : { kPageMotion, kPageColour })
                if (const int p = padPitch (pg, row, col); (p >= 12 && p < 24) || (p >= 5 && p <= 8))
                    noZones = false;
    check (noZones, "no zone or bar-selector pads on the Launchpad");
    e.padPressed (8, 8);                                   // Orange from the Motion page
    check (e.getSel (87) != 0 && e.getSel (85) == 0, "Motion colour replaces the Colour-page primary");
    e.padPressed (2, 1);                                   // Rainbow
    check (e.getSel (hitnotedmx::kColorDynStart) != 0, "Motion (2,1) latches Rainbow");
    e.padPressed (2, 1);                                   // Rainbow off again
    e.padPressed (8, 7);                                   // Red (Motion page) replaces Orange
    check (e.getSel (85) != 0 && e.getSel (87) == 0, "Motion Red replaces Motion Orange");

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

    // ── blackout (Launchpad top row) ──
    e.topButton (TopButton::Blackout, true);
    e.topButton (TopButton::Blackout, false);
    run (e, 0.2);
    {
        float sum = 0;
        for (int ch = 1; ch <= e.getRig().rigChannels(); ++ch) sum += e.getDmxValues().get (ch);
        check (sum < 1.0e-4f, "Blackout toggle kills the rig");
    }
    e.topButton (TopButton::Blackout, true);
    e.topButton (TopButton::Blackout, false);
    check (! e.isBlackout(), "Blackout toggles back off");

    // ── per-bar bump (XL top row) ──
    {
        const auto saved = e.toXml();             // keep the look for the checks below
        e.clearSelection();
        run (e, 0.5);
        const auto dark = barSum (e, 1);
        check (dark.r + dark.g + dark.b < 1.0e-4f, "nothing held = dark rig");

        e.buttonEvent (idx (B::BumpBar2), true);
        e.buttonEvent (idx (B::BumpBar2), false);  // a quick tap still fires
        run (e, 0.01);
        const auto b2 = barSum (e, 1), b1 = barSum (e, 0), b3 = barSum (e, 2);
        check (b2.r > 10.0f && b2.g > 10.0f && b2.b > 10.0f, "Bump 2 flashes bar 2 white");
        check (b1.r + b1.g + b1.b + b3.r + b3.g + b3.b < 1.0e-4f, "Bump 2 leaves bars 1 and 3 dark");

        e.setViewPage (kPageMotion);
        e.padPressed (8, 8);                       // Orange held → flash in orange
        e.buttonEvent (idx (B::BumpBar4), true);
        e.buttonEvent (idx (B::BumpBar4), false);
        run (e, 0.01);
        const auto b4 = barSum (e, 3);
        check (b4.r > 10.0f && b4.b < 1.0e-3f, "bar bump takes the held primary colour");

        // Orange alone lights the whole rig, so "back to the scene" = every
        // bar equal to an untouched one again.
        const auto sameAsBar1 = [&e] (int bar)
        {
            const auto a = barSum (e, 0), b = barSum (e, bar);
            return std::abs (a.r - b.r) + std::abs (a.g - b.g) + std::abs (a.b - b.b) < 1.0e-3f;
        };
        run (e, 3.0);                              // > 1 bar at the default release
        check (sameAsBar1 (1) && sameAsBar1 (3), "bar bumps decay back to the scene");

        e.buttonEvent (idx (B::BumpBar7), true);   // bar 7 on a 4-bar grid: nothing
        e.buttonEvent (idx (B::BumpBar7), false);
        run (e, 0.01);
        check (sameAsBar1 (1) && sameAsBar1 (2) && sameAsBar1 (3), "bump beyond the grid changes nothing");
        e.fromXml (*saved);
    }

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

        // Old saved state with a bar selector + zone latched: both dropped on load.
        juce::XmlElement old ("HitLaunchDmx");
        old.setAttribute ("sel", "2:127 6:127 14:127 85:127");
        LiveEngine e3 (dmx);
        e3.fromXml (old);
        check (e3.getSel (6) == 0 && e3.getSel (14) == 0 && e3.getSel (2) != 0 && e3.getSel (85) != 0,
               "loading drops bar selectors + zones, keeps the rest");
    }

    std::printf (failures == 0 ? "\nALL OK\n" : "\n%d FAILURE(S)\n", failures);
    return failures == 0 ? 0 : 1;
}
