# HitLaunchDmx — TODO

## Requested next (user-prioritised, 2026-06-12)

0. **Re-establish a link with the recipes developed in hitnotedmx.** The
   scene/effect code here (`Scene.h`, `Rig.h`, `SceneEngine.cpp`) forked from
   an earlier state; `../hitnotedmx/Source/Recipes.{h,cpp}` is the evolving
   reference. Share or re-port those recipes instead of maintaining a
   parallel fork — see CLAUDE.md for the options.

## Requested earlier (user-prioritised, 2026-06-05)

1. **Master parameters as Launchpad controls.** Map the four pots (DIM, TAIL,
   SPEED, XFADE) to physical Launchpad controls so they can be ridden without
   the mouse. Options: use the top round-button row (notes 91–98) as
   increment/decrement pairs per parameter, or — cleaner — put the unit's
   faders... (Mini MK3 has no faders, so use button pairs or a dedicated
   "control row" on the grid). Reflect the value back via LED brightness.
   Keep the mapping in the same editable layer as the pad→scene table.
   Touch points: `SceneEngine::handleIncomingMidiMessage` (decode control
   notes), new setters already exist (`setGlobalDim/SpeedScale/TailScale/
   CrossfadeSec`), and `MainComponent` pots should reflect hardware changes
   (add atomics getters + sync in the 15 Hz timer).

2. **Master-parameter reset.**
   - Per-pot reset: cmd-click and double-click on a pot resets it to its
     neutral default (DIM 100%, TAIL 100%, SPEED 100%, XFADE 2 s). In JUCE,
     `Slider::setDoubleClickReturnValue(true, defaultVal)` handles
     double-click; for cmd-click, handle in a `mouseDown` with
     `mods.isCommandDown()` and call `setValue(default)`.
   - "Reset all" on the Launchpad: pick a spare button (e.g. a top round
     button) that resets all four masters to neutral; add a matching UI
     button too.

3. **Better master-parameter ranges + window width.**
   - Some scenes are over-sensitive to SPEED/TAIL. Revisit ranges/curves:
     consider a skewed (logarithmic) SPEED slider so the useful range is
     spread out, and clamp/curve TAIL so extreme values don't wash a scene
     out. DIM may want a gentle gamma so low end is finer.
   - Make the window a tad wider so the virtual launchpad square spans the
     FULL window height (currently the square is limited by the right
     panel's available height; widen the window and/or right panel so
     `side = availH` and the grid fills top-to-bottom).

## Immediate / first use
- [ ] Smoke-test on Monterey/Intel target machine
- [ ] Confirm Launchpad note numbers via MIDI log (should be 11–88)
- [ ] Tune crossfade default (currently 2 s) — adjust slider to taste
- [ ] Tune scene `speed` values after watching each scene live on the rig
- [ ] Verify DMX channel assignment: bar_0 = DMX 1–54, bar_1 = 55–108, etc.

## Scene tuning
- [ ] Tweak MOON_RISE Gaussian sigma — currently `nPix * 0.25`; may want taller/shorter
- [ ] DRIFT_BALL: the ring gaussian `width` param is currently in scene `width` field;
      might need separate ring-width vs vertical-width params for some presets
- [ ] RING_COMET at slow speeds: comet is bar-wide (all 18 pixels lit equally);
      consider adding vertical pixel brightness gradient (brighter at top or centre)
- [ ] VERT_CHASE_STAGGER offset formula — currently `barIdx / nBars / speed`;
      verify it produces clean 90° offset at all speeds
- [ ] DIAGONAL_WIPE: currently uses `color1→color2` crossfade during the fill;
      may want a sharp edge instead for some scenes
- [ ] RAINBOW_RING: `vertOff * 0.1f` adds slight vertical hue variation;
      increase to taste (e.g. 0.2f for more rainbow-per-bar depth)

## New scenes / generators
- [ ] SPARKLE: per-pixel random twinkle (port from hitnotedmx, adapt to full RGB)
- [ ] WAVE_TRAIN: multiple overlapping sine comets with configurable spacing
- [ ] EXPAND_CONTRACT: ball / halo that breathes radially around the ring
- [ ] RAIN: pixels drop from top with random timing per bar
- [ ] Dual-colour RING_COMET (comet body = color1, tail fades to color2)

## Launchpad LED feedback
- [ ] Test LP Prog Mode SysEx on real hardware — may need adjusted SysEx bytes
      for MK3 vs MK2 (current: 0x00 0x20 0x29 0x02 0x0D 0x0E 0x01)
- [ ] Persist LED state across MIDI reconnect (re-send on open)
- [ ] Dim non-active pads by category more distinctively (currently all dim same)

## UI improvements
- [ ] Scene grid / pad map panel — visual 8×8 grid showing all 64 scene names
      so the user can see the layout without a printed cheat sheet
- [ ] "Rescan devices" button for MIDI and ENTTEC (currently only at startup)
- [ ] Show active scene index number alongside name
- [ ] Crossfade slider: add a "0 (instant)" notch at the low end

## Build / portability
- [ ] Test arm64 universal binary (add "arm64;x86_64" to CMAKE_OSX_ARCHITECTURES)
- [ ] Sign with a Developer ID certificate for distribution without quarantine flag
- [ ] Confirm ENTTEC driver on Apple Silicon (IOKit deprecation warnings are
      cosmetic on Ventura+; the kIOMasterPortDefault → kIOMainPortDefault rename
      is Ventura-only — current code targets Monterey so kIOMasterPortDefault is correct)
