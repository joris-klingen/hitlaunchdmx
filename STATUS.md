# HitLaunchDmx — status & architecture

A **standalone macOS app** (JUCE 8, CMake) for playing the hitdmx rig live
without a DAW:

- a **Novation Launchpad Mini MK3** picks *what* is lit — recipes, colours,
  spots — and stores/recalls whole looks;
- a **Novation Launch Control XL** (Factory template 1) rides *how* —
  per-bar dims on the faders, bump/strobe/freeze/… on the buttons, tempo,
  relative speed, tail and the other parameters on the knobs;
- output goes to an **ENTTEC DMX USB Pro**.

**The look engine is hitnotedmx's, not a copy.** The grid, palettes, all
recipes, the composition (`computeDmx`), the ENTTEC driver and the visualiser
are compiled straight from `../hitnotedmx/Source` (see `CMakeLists.txt`,
`HITNOTEDMX_DIR`). hitnotedmx itself is not modified. Anything that changes
there shows up here on the next build.

## How it works

hitnotedmx turns *held MIDI notes + their velocities* into a DMX frame. This
app is just a different way of holding those notes:

```
Launchpad (MIDI thread) ─┐                 ┌─ XL (MIDI thread)
  pads → latch a note     │                 │  knobs/faders → params (0..1)
  page / Store / Shift    ▼                 ▼  buttons → momentary/toggle
                       LiveEngine  (atomics in, one SpinLock for looks)
                           │
         100 Hz tick ──────┤  1. clock: global beats @ BPM; scaled clock =
         (own thread)      │     global × relative speed (switches on a beat)
                           │  2. held-note set: latched pads + XL buttons,
                           │     velocity = the XL knob for that bank
                           │  3. hitnotedmx::computeDmx (unchanged)
                           │  4. strobe → driver shutter; channels → ENTTEC
                           │  5. publish frame for the GUI
                           ▼
GUI timer (30 Hz): Launchpad + XL LED feedback (only on change), virtual
Launchpad + virtual XL panel, visualiser, tempo readout, autosave.
```

### Velocity = knob

| Held note (hitnotedmx) | Velocity comes from |
|---|---|
| Chases 24–35 | XL *Chase tail* |
| Breathes 36–47 | XL *Breathe* (glow-mask depth) |
| Wild 49–59 | XL *Wild div* (beat division) |
| Strobe 48 | XL *Strobe Hz* (also the driver shutter rate) |
| Multicolor 60–83 | XL *Multi spd* |
| Palette 84–119 | XL *Col fade* (0 = instant) |
| Spots 1–4, blackout 0 | 127 |
| Bump 120 / Release 121 | *Bump lvl* / *Release* (Bump = XL *Bump all*) |
| Crossfade 122 / Spread 126 | knob; held only when > 0 |
| To/From black 10/9 | *Blk glide* (0 = instant) |
| Freeze/Reverse/Flip 123–125 | 127 while toggled on |
| **Speed 127** | **never held** — replaced by the relative speed knob |

### Tempo

No host: a global beat clock runs at the XL **BPM** knob (60–180) or the
**Tap** button (rolling average of the last 4 taps; every tap also lands the
beat on the tap). **Rel speed** (¼ ½ 1 2 4×) multiplies the clock handed to
`computeDmx`; a change takes effect on the next global beat and the scaled
clock is integrated (never jumps), so motion stays continuous and beat-locked.
Beat-based lengths (bump release, crossfade) scale with it — agreed.
**Restart** re-locks everything to beat 0 (like transport start in hitnotedmx).

## Controls

### Launchpad Mini MK3 (Programmer mode)

| | |
|---|---|
| Right column (top → bottom) | page: **Motion**, **Colour**, **Looks 1–6** |
| Motion page (all-in-one) | cols 1-6: rows 8-7 Chases · 6-5 Breathes · 4-3 Wild (each bank 6 × 2: upper row = notes 0-5, lower 6-11) · cols 7-8, rows 8-3: 12 primary colours (Red, Orange · Yellow, Green · Teal, Cyan · Blue, Violet · Magenta, Pink · Warm white, Cool white) · rows 2-1: 16 Multicolor (Rainbow … Night sky). Picks are `kMotionColours` / `kMotionMulticolor` in `PadLayout.h`. No zones or bar selectors on the Launchpad — all bars always play, the XL faders set their levels |
| Colour page | rows 8-6 Primary 24 · row 5 + row 4 cols 1-4 Secondary 12 · rows 3-1 Multicolor 24 |
| Looks pages | 64 looks each (384 total) |
| Top row | Spot L WW · Spot L col · Spot R WW · Spot R col · Clear · Store · Shift · Blackout |

Tap a pad → it replaces the rest of its bank (one chase, one primary, …);
**Shift + tap** adds (recipes combine as in hitnotedmx); tap a lit pad → off.
**Store** then a look pad → save; **Shift + Store** then a pad → delete; a
look pad → recall (crossfades if the XL *Crossfade* knob is up).

### Launch Control XL (Factory template 1, MIDI ch 9)

The full table lives in `Source/Controls.h` (the one place to remap).

| Strip | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|
| Send A | BPM | Rel speed | Chase tail | Breathe | Wild div | Multi spd | Spread | Density |
| Send B | Release | Bump lvl | Crossfade | Col fade | – | Blk glide | – | Spot dim |
| Pan | – | – | – | – | – | – | – | Strobe Hz |
| Fader | Bar 1 | Bar 2 | Bar 3 | Bar 4 | Bar 5 | Bar 6 | Bar 7 | **Master** (LED dim) |
| Focus btn | **Bump 1** | **Bump 2** | **Bump 3** | **Bump 4** | **Bump 5** | **Bump 6** | **Bump 7** | Blackout |
| Control btn | Tap | Restart | Reverse | From black | To black | Freeze | Flip | **Strobe** |

Strips 1–7 are the bars (fader = bar dim, top button = bar bump); **strip 8
is the master column** — Density, Spot dim, Strobe Hz, the Master fader,
Blackout over Strobe. Blackout is one state shared with the Launchpad's
top-row Blackout. Saved state from the earlier layout is migrated on load
(`paramLayout` attribute).

Bumps and Strobe are momentary, Tap/Restart one-shots, the rest toggle.
**Bump 1–7** are per-bar bumps: the
same flash as hitnotedmx's Bump (fires on the press, decays over *Release*,
toward white or the held primary colour, at *Bump lvl*, scaled by that bar's
fader and *LED dim*) on one bar only. hitnotedmx's Bump is whole-rig, so the
per-bar flash is done in this app (`LiveEngine::tick` step 4) on a copy of the
composed frame, which keeps it working under Freeze. A bump for a bar the grid
doesn't have does nothing (LED off). Faders
beyond the grid's bar count do nothing (greyed on screen).

## Files

| File | Role |
|---|---|
| `Source/Controls.h` | XL layout: params, buttons, value formatting, factory-template MIDI numbers + LED codes |
| `Source/PadLayout.h` | Launchpad pages: pad ↔ hitnotedmx note, look slots, bank groups, top buttons |
| `Source/LiveEngine.{h,cpp}` | State (atomics), clock, held-note building, `computeDmx` tick, persistence |
| `Source/Surfaces.{h,cpp}` | Launchpad + XL MIDI I/O and LED feedback; `describeLaunchpad` / `describeXlButton` shared with the on-screen mirrors |
| `Source/MainComponent.{h,cpp}` | Window (1460 × 880, sized for a 14" MBP): visualiser, virtual Launchpad, virtual XL panel, devices, grid, tempo, log |
| `tools/EngineCheck.cpp` | Headless CTest: drives `LiveEngine::tick` and checks latching, routes, faders, clock, tap, looks, persistence |
| `../hitnotedmx/Source/*` | Rig, Palette, Recipes, Composition, MidiState, EnttecProDmx, DmxVisualizer, TriggerVocabulary |

State (grid, all params, current selection, looks) autosaves ~3 s after a
change and on quit to `~/Library/Application Support/HitLaunchDmx/state.xml`.

## Build & verify

```sh
cmake -S . -B build_dev -G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build_dev --config Release -j8
ctest --test-dir build_dev -C Release
```

Release builds are Apple Silicon (arm64) only; a universal build is possible if ever needed:
`cmake -S . -B build_universal -G Xcode "-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64"`.
`installer/install.command` copies the app to /Applications and strips the
quarantine flag.

## Changelog

- **2026-10 — XL strip 8 = master column:** Density, Spot dim (Send B 8),
  Strobe Hz (Pan 8), Master dim (fader 8), Blackout (top 8) over Strobe
  (bottom 8). Reverse → bottom 3; Bump all dropped (bar bumps 1–7 remain);
  grids now have bar faders for bars 1–7. Old saved params migrate.
- **2026-10 — XL buttons reworked:** top row = per-bar bumps 1–7 + Bump all;
  bottom row = Tap, Restart, Strobe, From black, To black, Freeze, Flip,
  Reverse. Blackout moved to the Launchpad only.
- **2026-10 — Motion page is the all-in-one play page:** zones removed from
  the Launchpad; 12 colours in the right two columns (rows 8-3) and 16
  Multicolor looks across the bottom two rows. Zones and bar selectors are
  off the Launchpad (bar levels = XL faders) and dropped from old saved
  state on load.
- **2026-10 — rebuilt on hitnotedmx's engine.** The forked scene engine
  (`Scene.h`, `SceneEngine`, own `Rig`/`Palette`/ENTTEC/visualiser copies, 128
  fixed scenes) is gone. The app now holds hitnotedmx notes: Launchpad builds
  the look, Launch Control XL rides every parameter, internal BPM + tap +
  relative speed replace the host clock, on-screen mirrors of both surfaces.
- 2026-08 — grid-style rig (spots at DMX 1–12, bars from 13) to match
  hitnotedmx's patch (checkpoint commit, superseded by the above).
- Earlier history: see git log.
