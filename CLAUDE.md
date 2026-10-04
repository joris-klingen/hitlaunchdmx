# hitlaunchdmx

Standalone macOS app (JUCE 8, CMake) for playing the hitdmx rig live: a
Launchpad Mini MK3 builds/recalls the look, a Launch Control XL (Factory
template 1) rides every parameter, output via an ENTTEC DMX USB Pro. Part of
the hitdmx family (sibling repos in `../`): **hitnotedmx** (MIDI-notes→DMX
VST3, the backbone), **hitccdmx** (raw CC-style channel VST), **hitdesigndmx**
(clip design + legacy conversion).

Architecture, control maps and current state: [STATUS.md](STATUS.md). Work
queue: [TODO.md](TODO.md).

## Invariants

- **The look engine is hitnotedmx's.** Rig, palette, recipes, `computeDmx`,
  ENTTEC driver, visualiser and vocabulary are compiled from
  `../hitnotedmx/Source` (`HITNOTEDMX_DIR`). Never copy or fork them here; new
  recipe/look behaviour belongs in hitnotedmx. Don't modify hitnotedmx from
  this repo's work without asking — it is show-critical.
- This app only decides **which hitnotedmx notes are held and at what
  velocity** (`LiveEngine::tick`). hitnotedmx's Speed note (127) is never held;
  the relative-speed knob scales the clock instead.
- `Source/Controls.h` is the single XL mapping table; `Source/PadLayout.h` the
  single Launchpad layout. Hardware LEDs and on-screen mirrors both read
  `describeLaunchpad` / `describeXlButton`, so they can't drift.
- The engine tick runs on its own thread: no allocations, no locks it can
  block on (inputs are atomics; looks use a SpinLock the tick never takes).

## Build & verify

```sh
cmake -S . -B build_dev -G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build_dev --config Release -j8
ctest --test-dir build_dev -C Release
```

(The older `build/`, `build_native/`, `build_universal/` dirs were configured
at the pre-reorg path `code/hitlaunchdmx` and won't reconfigure in place.)

## Releasing to the show machine

The show machine (M1 — arm64 only, no Intel/universal build) installs from the
Dropbox folder
`~/Library/CloudStorage/Dropbox/Music/Hitmix/9_tech/hitlaunch_installer`
(`HitLaunchDmx_installer/` = `HitLaunchDmx.app` + `install.command`, plus a
zipped copy `HitLaunchDmx_installer.zip`). **Refresh it only on request**:
build + `ctest`, `ditto` the app and [installer/install.command](installer/install.command)
into the folder, then re-zip with `ditto -c -k --keepParent`.
