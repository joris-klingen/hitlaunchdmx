# HitLaunchDmx — TODO

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
