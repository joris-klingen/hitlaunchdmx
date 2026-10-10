// Headless check of the live engine: drives LiveEngine::tick with a synthetic
// clock (no GUI, no MIDI devices, ENTTEC not connected) and asserts that the
// Launchpad/XL state turns into the expected hitnotedmx output.
//
// Run: ctest --test-dir build_dev -C Release   (or the engine-check binary)

#include <cmath>
#include <cstdio>

#include "AutoPrograms.h"
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

// A quick pad tap (press + release) at the synthetic time.
void tapPad (LiveEngine& e, int row, int col)
{
    e.padPressed (row, col, now);
    e.padReleased (row, col, now + 0.05);
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
    tapPad (e, 8, 1);                                 // Chases, first pad = Chase
    check (e.getSel (hitnotedmx::kChasesStart) != 0, "motion pad (8,1) latches Chase (24)");
    e.setViewPage (kPageColour);
    tapPad (e, 8, 2);                                 // Primary #1 = Red (85)
    check (e.getSel (hitnotedmx::kPrimaryPaletteStart + 1) != 0, "colour pad (8,2) latches Red (85)");

    run (e, 2.0);
    float red = 0, other = 0;
    for (int b = 0; b < 4; ++b) { const auto s = barSum (e, b); red += s.r; other += s.g + s.b; }
    check (red > 0.5f, "red chase lights the bars");
    check (other < 1.0e-4f, "red chase emits no green/blue");

    // ── group exclusivity / hold-to-combine ──
    tapPad (e, 8, 3);                                      // Orange-red replaces Red
    check (e.getSel (85) == 0 && e.getSel (86) != 0, "new primary replaces the old one");
    e.padPressed (8, 4, now);                              // hold Orange (replaces Orange-red) …
    tapPad (e, 8, 2);                                      // … and tap Red while it's held
    e.padReleased (8, 4, now);
    check (e.getSel (85) != 0 && e.getSel (87) != 0 && e.getSel (86) == 0, "holding a pad + tapping another adds");
    tapPad (e, 8, 2);
    check (e.getSel (85) == 0, "tap without a held pad toggles off");
    tapPad (e, 8, 2);                                      // Red back on, replacing Orange
    check (e.getSel (85) != 0 && e.getSel (87) == 0, "tap without a held pad replaces");
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
    tapPad (e, 8, 8);                                   // Orange from the Motion page
    check (e.getSel (87) != 0 && e.getSel (85) == 0, "Motion colour replaces the Colour-page primary");
    tapPad (e, 2, 1);                                   // Rainbow
    check (e.getSel (hitnotedmx::kColorDynStart) != 0, "Motion (2,1) latches Rainbow");
    tapPad (e, 2, 1);                                   // Rainbow off again
    tapPad (e, 8, 7);                                   // Red (Motion page) replaces Orange
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

    // ── blackout (XL top 8) ──
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
        tapPad (e, 8, 8);                       // Orange held → flash in orange
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

    // ── master column (strip 8) ──
    check (xl::paramForCc (xl::kFaderCc[7]) == idx (P::LedMaster), "fader 8 = master dim");
    check (xl::paramForCc (xl::kPanCc[7]) == idx (P::StrobeRate), "pan 8 = strobe speed");
    check (xl::paramForCc (xl::kSendBCc[7]) == idx (P::SpotMaster), "send B 8 = spot dim");
    check (xl::buttonForNote (xl::kFocusNote[7]) == idx (B::Blackout), "top 8 = Blackout");
    check (xl::buttonForNote (xl::kControlNote[7]) == idx (B::Strobe), "bottom 8 = Strobe");
    check (xl::buttonForNote (xl::kControlNote[2]) == idx (B::Reverse), "bottom 3 = Reverse");

    e.buttonEvent (idx (B::Blackout), true);                 // XL Blackout …
    e.buttonEvent (idx (B::Blackout), false);
    check (e.isBlackout() && e.isButtonOn (idx (B::Blackout)), "XL Blackout sets the shared blackout");
    e.buttonEvent (idx (B::Blackout), true);                 // … and off again
    e.buttonEvent (idx (B::Blackout), false);
    check (! e.isBlackout() && ! e.isButtonOn (idx (B::Blackout)), "XL Blackout toggles off");

    run (e, 0.3);
    {
        float before = 0;
        for (int b = 0; b < 4; ++b) { const auto x = barSum (e, b); before += x.r + x.g + x.b; }
        e.setParam (idx (P::LedMaster), 0.0f);
        run (e, 0.3);
        float after = 0;
        for (int b = 0; b < 4; ++b) { const auto x = barSum (e, b); after += x.r + x.g + x.b; }
        check (before > 1.0f && after < 1.0e-4f, "master fader at 0 darkens every bar");
        e.setParam (idx (P::LedMaster), 1.0f);
    }

    e.setParam (idx (P::StrobeRate), 1.0f);
    e.buttonEvent (idx (B::Strobe), true);
    run (e, 0.05);
    check (std::abs (e.getStrobeHz() - static_cast<float> (hitnotedmx::kStrobeMaxHz)) < 0.01f, "Strobe at full speed = 20 Hz");
    e.buttonEvent (idx (B::Strobe), false);
    run (e, 0.05);
    check (e.getStrobeHz() == 0.0f, "Strobe off on release");
    e.setParam (idx (P::StrobeRate), 0.5f);

    // ── saved state from before the master column is migrated ──
    {
        juce::StringArray old;
        for (int i = 0; i < kNumParams; ++i) old.add ("0.5");
        old.set (12, "0.9");    // old Strobe Hz (Send B 5)
        old.set (16, "0.3");    // old LED dim (Pan 1)
        old.set (17, "0.7");    // old Spot dim (Pan 2)
        old.set (31, "0.1");    // old bar 8 dim
        juce::XmlElement x ("HitLaunchDmx");
        x.setAttribute ("params", old.joinIntoString (" "));
        LiveEngine m (dmx);
        m.fromXml (x);
        check (std::abs (m.getParam (idx (P::StrobeRate)) - 0.9f) < 1e-4f
            && std::abs (m.getParam (idx (P::LedMaster))  - 0.3f) < 1e-4f
            && std::abs (m.getParam (idx (P::SpotMaster)) - 0.7f) < 1e-4f
            && std::abs (m.getParam (idx (P::Bpm))        - 0.5f) < 1e-4f,
               "old param layout migrates to the master column");
    }

    // ── looks: store + recall ──
    e.setViewPage (kFirstLookPage);
    e.padPressed (8, 1, now);
    e.pollHolds (now + 0.3);
    check (! e.lookUsed (0), "a short hold on a look pad doesn't store");
    e.pollHolds (now + kLookHoldSec + 0.05);
    e.padReleased (8, 1, now + kLookHoldSec + 0.1);
    check (e.lookUsed (0) && e.getActiveLook() == 0, "holding a look pad stores look 1");
    e.clearSelection();
    check (e.getSel (85) == 0, "Clear empties the selection");
    tapPad (e, 8, 1);
    check (e.getSel (85) != 0 && e.getSel (hitnotedmx::kChasesStart) != 0, "look pad recalls the stored look");

    // ── persistence round-trip ──
    {
        auto xml = e.toXml();
        LiveEngine e2 (dmx);
        e2.fromXml (*xml);
        check (e2.lookUsed (0) && e2.getSel (85) != 0, "state survives toXml/fromXml");

        // Old saved state with a bar selector + zone latched: both dropped on load.
        juce::XmlElement old ("HitLaunchDmx");
        old.setAttribute ("sel", "0:127 2:127 6:127 14:127 85:127");
        LiveEngine e3 (dmx);
        e3.fromXml (old);
        check (e3.getSel (0) == 0 && e3.getSel (6) == 0 && e3.getSel (14) == 0 && e3.getSel (2) != 0 && e3.getSel (85) != 0,
               "loading drops blackout note, bar selectors + zones, keeps the rest");
    }

    // ── hold a page button = clear ──
    e.pageButton (kPageMotion, true, now);
    e.pollHolds (now + 0.5);
    check (e.getSel (85) != 0, "a short page press doesn't clear");
    e.pollHolds (now + kClearHoldSec + 0.05);
    e.pageButton (kPageMotion, false, now + kClearHoldSec + 0.1);
    check (e.getSel (85) == 0 && e.getSel (hitnotedmx::kChasesStart) == 0, "holding a page button clears");

    // ── auto modes ──
    {
        const auto& lv = autoLevels();
        const auto holds = [&e] (const AutoProgram& prog)
        {
            for (const int n : prog.notes)
                if (n >= 0 && e.getSel (n) == 0) return false;
            return true;
        };
        e.topButton (TopButton::SpotLWW, true);           // a spot on — auto must keep it
        e.topButton (TopButton::SpotLWW, false);
        e.setParam (idx (P::Bpm), normFromBpm (120.0));   // exact (the tap test left ~120)
        e.restartClock();
        run (e, 0.02);
        // Every program note is a real recipe/colour note (never 0 = Blackout).
        bool notesOk = true;
        for (const auto& L : lv)
            for (int i = 0; i < L.numPrograms; ++i)
                for (const int n : L.programs[static_cast<size_t> (i)].notes)
                    if (n != -1 && n < hitnotedmx::kChasesStart) notesOk = false;
        check (notesOk, "auto programs hold only recipe/colour notes (no Blackout)");

        // Mess everything up first: auto must reset it all.
        e.buttonEvent (idx (B::Flip), true);      e.buttonEvent (idx (B::Flip), false);
        e.buttonEvent (idx (B::Freeze), true);    e.buttonEvent (idx (B::Freeze), false);
        e.buttonEvent (idx (B::ToBlack), true);   e.buttonEvent (idx (B::ToBlack), false);
        e.buttonEvent (idx (B::Blackout), true);  e.buttonEvent (idx (B::Blackout), false);
        e.setParam (idx (P::Spread), 0.6f);
        e.setParam (idx (P::RelSpeed), 1.0f);
        e.setParam (idx (P::ChaseTail), 0.05f);
        e.setParam (idx (P::BreatheDepth), 0.1f);
        e.setParam (idx (P::WildDiv), 1.0f);
        e.setParam (idx (P::ColorSpeed), 0.0f);

        e.topButton (TopButton::Auto1, true);             // Mellow
        e.topButton (TopButton::Auto1, false);
        run (e, 0.05);
        check (! e.isButtonOn (idx (B::Flip)) && ! e.isButtonOn (idx (B::Freeze))
                 && ! e.isButtonOn (idx (B::ToBlack)) && ! e.isBlackout()
                 && e.getParam (idx (P::Spread)) == 0.0f && std::abs (e.getParam (idx (P::RelSpeed)) - 0.5f) < 1e-6f,
               "starting an auto resets toggles, blackout, spread and rel speed");
        check (e.getParam (idx (P::ChaseTail)) == kParamDefs[static_cast<size_t> (idx (P::ChaseTail))].deflt
                 && e.getParam (idx (P::BreatheDepth)) == kParamDefs[static_cast<size_t> (idx (P::BreatheDepth))].deflt
                 && e.getParam (idx (P::WildDiv)) == kParamDefs[static_cast<size_t> (idx (P::WildDiv))].deflt
                 && e.getParam (idx (P::ColorSpeed)) == kParamDefs[static_cast<size_t> (idx (P::ColorSpeed))].deflt,
               "starting an auto resets the recipe knobs (chase tail, breathe, wild, multi) to defaults");
        check (e.getSel (0) == 0, "auto never latches the Blackout note");
        run (e, 1.0);
        {
            float lit = 0;
            for (int b = 0; b < 4; ++b) { const auto x = barSum (e, b); lit += x.r + x.g + x.b; }
            check (lit > 1.0f, "auto program actually lights the rig");
        }
        check (e.getAutoLevel() == 0 && e.getAutoProgram() == 0 && holds (lv[0].programs[0]),
               "Auto 1 starts Mellow program 1");
        run (e, lv[0].barsPerProgram * 4 * 0.5 + 0.1);    // 8 bars at 120 BPM
        check (e.getAutoProgram() == 1 && holds (lv[0].programs[1]) && ! holds (lv[0].programs[0]),
               "Mellow moves to program 2 after 8 bars");
        check (e.getSel (kSpotPitch0) != 0, "auto leaves the spots alone");

        e.topButton (TopButton::Auto4, true);             // switch straight to Disco
        e.topButton (TopButton::Auto4, false);
        run (e, 0.05);
        check (e.getAutoLevel() == 3 && holds (lv[3].programs[0]), "Auto 4 switches to Disco program 1");
        run (e, 4 * 0.5 + 0.1);                           // 1 bar
        check (e.getAutoProgram() == 1 && holds (lv[3].programs[1]), "Disco changes every bar");

        e.topButton (TopButton::Auto4, true);             // same button again = stop
        e.topButton (TopButton::Auto4, false);
        run (e, 0.05);
        check (e.getAutoLevel() == -1 && holds (lv[3].programs[1]), "same auto button stops, look stays");

        e.topButton (TopButton::Auto3, true);             // Party: Chase + Red, bump each downbeat
        e.topButton (TopButton::Auto3, false);
        run (e, 0.05);
        {
            // Auto adds nothing on top of a program (no automatic bumps): with
            // Party's Chase + Red at tail 0, no bar ever lights fully, even
            // across downbeats (an auto bump used to flash a whole bar there).
            e.setParam (idx (P::ChaseTail), 0.0f);
            e.setParam (idx (P::BumpLevel), 1.0f);
            float worst = 0;
            for (int i = 0; i < 300; ++i)                  // 3 s ≈ 6 beats, 1+ downbeat
            {
                run (e, 0.01);
                if (e.getAutoProgram() != 0) break;
                for (int b = 0; b < 4; ++b) worst = juce::jmax (worst, barSum (e, b).r);
            }
            check (worst < 8.0f, "auto fires no bumps on top of the programs");
        }
        e.setViewPage (kPageMotion);
        tapPad (e, 8, 1);                                 // any manual pad stops auto
        check (e.getAutoLevel() == -1, "a manual pad stops auto");
    }

    std::printf (failures == 0 ? "\nALL OK\n" : "\n%d FAILURE(S)\n", failures);
    return failures == 0 ? 0 : 1;
}
