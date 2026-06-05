# HitLaunchDmx — status & architecture

A **standalone macOS app** (JUCE 8, CMake) that takes MIDI note triggers from a
**Novation Launchpad Mini MK3** and plays smooth, free-running ambient lighting
scenes on a ring of 4 LED bars via an **ENTTEC DMX USB Pro**. Nothing is
tempo-synced; all motion runs on wall-clock time with smooth crossfades between
scenes.

Target hardware: Intel i7 Mac, macOS Monterey 12.0.

---

## Architecture

```
MIDI callback thread
  handleIncomingMidiMessage()
    → noteToSceneIndex(note)      [Scene.h — O(1) lookup]
    → activeScene.store(idx)      [std::atomic<int>]
    → midiLog.push(entry)         [lock-free SPSC ring]

60 Hz HighResolutionTimer (SceneEngine)
  → renderPixel(scene, wallClockSec, bar, pixel) → RGB
  → LERP liveFrame toward targetFrame (exponential, configurable τ)
  → dmx.setChannel × 216         [EnttecProDmx, sends at 40 Hz]
  → liveFrameOut.set × 216       [DmxFrame, atomic uint8_t, GUI read]

15 Hz GUI Timer (MainComponent)
  → drain midiLog → TextEditor (capped at 20 lines)
  → visualizer.repaintIfChanged()
     ↳ fingerprint diff → rebuildCache()
     ↳ repaint() → paint() blits juce::Image
```

---

## Files

| File | Role |
|---|---|
| `CMakeLists.txt` | `juce_add_gui_app`, x86_64, macOS 12.0, MIDI in+out |
| `Source/Rig.h` | 4 bars × 18 px = 216 DMX channels; ring angles (12/3/6/9 o'clock) |
| `Source/Palette.h` | Named `PaletteColor` constants + Launchpad LED colour bytes |
| `Source/Scene.h` | `MotionType` enum, `Scene` struct, `kScenes[64]` preset table, `noteToSceneIndex()` |
| `Source/SceneEngine.{h,cpp}` | 60 Hz render loop, crossfade LERP, MIDI input/output, Launchpad LED feedback |
| `Source/DmxVisualizer.{h,cpp}` | Ring preview (4 bars at clock positions), fingerprint-diff repaint |
| `Source/MainComponent.{h,cpp}` | 3-pane layout: left (MIDI device + DMX controls + MIDI log), centre (visualiser), right (scene name + crossfade slider) |
| `Source/Main.cpp` | `JUCEApplication` + `DocumentWindow` |
| `Source/EnttecProDmx.{h,cpp}` | Self-contained ENTTEC DMX USB Pro driver (IOKit + termios, 40 Hz send) |
| `Source/MidiLog.{h,cpp}` | Lock-free SPSC ring (256 entries) for MIDI → GUI handoff |

---

## Scene model

**11 parametric generators** (pure functions of wall-clock time `t`, bar index,
pixel index):

| Generator | What it does |
|---|---|
| `BREATHE` | Global sinusoidal dim |
| `MOON_RISE` | Gaussian glow climbing vertically |
| `DRIFT_BALL` | Gaussian blob drifting around the ring |
| `RING_ROTATE` | Bar-wide highlight sweeping the ring |
| `RING_COMET` | Comet with tail travelling bar-by-bar around the ring |
| `VERT_CHASE` | Vertical comet, all bars in phase |
| `VERT_CHASE_STAGGER` | Vertical comet, each bar offset 90° from the previous |
| `COLOUR_WASH` | Sinusoidal cross-fade between color1 and color2 across bars |
| `PULSE_RING` | Sinusoidal radial pulse travelling the ring |
| `RAINBOW_RING` | HSV hue walking around the ring over time |
| `DIAGONAL_WIPE` | Vertical fill staggered per bar |

**64 scenes** in `kScenes[]`:

| Pad rows | Scene indices | Category |
|---|---|---|
| 1–4 (notes 11–48) | 0–31 | 32 mild breathes |
| 5–6 (notes 51–68) | 32–47 | 16 medium chases |
| 7–8 (notes 71–88) | 48–63 | 16 wild chases |

Pad → scene: `row = note/10 - 1 (0=bottom)`, `col = note%10 - 1`, `idx = row*8 + col`.

---

## Crossfade

Exponential LERP: each tick `liveFrame[ch] += (target[ch] - live[ch]) × α`
where `α = clamp(dt × lerpRate, 0, 1)` and `lerpRate = 1 / crossfadeSec`
(default 2 s). This gives a smooth approach regardless of how discontinuous
the scene change is.

---

## Launchpad setup

The app targets **Programmer mode** (fixed note layout: note = row×10 + col,
grid pads 11–88). Click **LP Prog Mode** in the UI to send the SysEx command
that switches the unit into Programmer mode.

On scene change, the engine sends note-on messages to all 64 grid pads:
- Active pad: velocity 127 (bright white)
- Category idle colours: rows 1–4 amber dim, rows 5–6 blue dim, rows 7–8 green dim

---

## Build

```bash
cmake -S . -B build -G Xcode
cmake --build build --config Release
```

App lands at `build/HitLaunchDmx_artefacts/Release/HitLaunchDmx.app`.

On first launch on the target Mac (Monterey/Intel):
```bash
xattr -dr com.apple.quarantine /path/to/HitLaunchDmx.app
open /path/to/HitLaunchDmx.app
```

Verified: `lipo -archs` → `x86_64`; `otool -l … | grep minos` → `12.0`.

---

## Verification checklist

- [ ] App launches on macOS 12 / Intel — no quarantine crash
- [ ] MIDI log shows incoming notes from Launchpad (confirm note numbers match 11–88)
- [ ] Pressing a pad changes the active scene (scene name updates in UI)
- [ ] Visualiser shows smooth crossfades between scenes around the ring
- [ ] Click "LP Prog Mode" → Launchpad LEDs light per category colour
- [ ] Active pad lights bright; others dim to category colour on scene change
- [ ] Connect ENTTEC USB Pro → "Connected" in status; DMX output on channels 1–216
- [ ] Blackout button cuts all output
- [ ] No audio hardware required
