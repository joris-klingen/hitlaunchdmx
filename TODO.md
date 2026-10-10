# HitLaunchDmx — TODO

## First use on hardware (needs the real devices)
- [ ] Launchpad: confirm Programmer-mode SysEx + RGB LED SysEx on the Mini MK3,
      pad notes 11–88, top row CC 91–98, right column CC 19–89.
- [ ] Launch Control XL: confirm Factory template 1 = MIDI ch 9 with the CC/note
      numbers in `Source/Controls.h`, and that the LED SysEx (template id 8)
      lights knobs + buttons. The MIDI log in the left panel shows raw input.
- [ ] ENTTEC smoke test on the rig (same patch as hitnotedmx: spots 1–12, bars from 13).
- [ ] Test the arm64 build on the show machine (M1).

## Feel / tuning (after playing it)
- [ ] Knob ranges/curves: BPM range 60–180, Release default, Col fade.
- [ ] Spare controls: Send B 7–8, Pan 3–8, Control buttons 3–8, XL side +
      arrow buttons. Candidates: bump-to-colour, secondary-only bump, look
      crossfade length, page select on the XL arrows.
- [ ] Soft takeover for XL knobs after a look recall/restore (knobs are
      absolute; the app jumps to the knob position on first move).
- [ ] Launchpad: momentary "flash" pads (hold-to-show a colour/recipe)?

## Engine sharing
- [ ] hitnotedmx is compiled from `../hitnotedmx` (always its current working
      tree). If that ever gets risky, pin it (git submodule or a tag check in
      CMake).
