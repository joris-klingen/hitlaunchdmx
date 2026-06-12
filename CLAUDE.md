# hitlaunchdmx

Standalone macOS app (JUCE 8, CMake): Launchpad Mini MK3 pads trigger
free-running ambient lighting scenes on 4 LED bars via an ENTTEC DMX USB Pro.
Part of the hitdmx family (sibling repos in `../`): **hitnotedmx**
(MIDI-notes→DMX VST3, the backbone), **hitccdmx** (raw CC-style channel VST),
**hitdesigndmx** (clip design + legacy conversion).

Architecture and current state: see [STATUS.md](STATUS.md). Work queue:
[TODO.md](TODO.md).

## Key pending direction (2026-06-12)

**Re-establish a link with the recipes developed in hitnotedmx.** This app's
scene/effect code (`Source/Scene.h` QUAD_* recipes, `Source/Rig.h` motion
recipes, `Source/SceneEngine.cpp`) forked from an earlier state and has
diverged, while the recipe bank in `../hitnotedmx/Source/Recipes.{h,cpp}`
(plus `Composition.*`, `Palette.h`) kept evolving and is the reference
implementation. hitlaunchdmx should consume/share those recipes again rather
than maintain a parallel fork — whether by extracting a shared library the
two apps both build against, or by porting hitnotedmx's recipes over and
keeping them aligned. Decide the mechanism with the user before restructuring;
hitnotedmx is the backbone and must not be destabilised by the sharing.

Note: `Source/EnttecProDmx.{h,cpp}` is likewise a copy shared by family
plugins — a candidate for the same shared library if one is created.
