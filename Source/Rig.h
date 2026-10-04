#pragma once

#include <array>
#include <cstdint>

#include "EnttecProDmx.h"  // for kDmxUniverseSize

namespace hitlaunchdmx
{

// Grid-style rig, adopted from hitnotedmx so both apps drive the SAME physical
// DMX patch. Two RGBW spots are pinned at the BOTTOM of the universe (DMX
// 1-12) and the vertical bars follow contiguously from DMX 13:
//
//   spot_l : DMX  1- 6   (Dim, R, G, B, W, Strobe)
//   spot_r : DMX  7-12
//   bar_0  : DMX 13- 66   (18 px × 3, pixel 1 RGB = ch 13..15, bottom-up)
//   bar_1  : DMX 67-120
//   bar_2  : DMX 121-174
//   bar_3  : DMX 175-228
//
// The grid is fixed at 4 bars × 18 pixels (hitnotedmx's default shape).
// hitlaunchdmx's launchpad recipes do not model the spots — those channels are
// RESERVED so the bar addressing lines up with hitnotedmx, and the spots stay
// dark (never written, so the driver holds them at 0).
//
// Motion recipes still treat the four bars as ONE long vertical line of 72
// LEDs (see gidFor / kNumLeds below), so nothing in Scene.h / SceneEngine.cpp
// changes: only the base DMX address of the bars moved from 1 to 13.

inline constexpr int kNumSpots       = 2;
inline constexpr int kSpotChannels   = 6;
inline constexpr int kBarChannelBase = kNumSpots * kSpotChannels + 1;   // 13

inline constexpr int kNumBars      = 4;
inline constexpr int kPixelsPerBar = 18;

// The 4 bars are stacked into ONE long vertical line of 72 LEDs.
// Global index (gid) runs 0..71 bottom→top:
//   bar 0 pixel 1  = gid 0   (very bottom)
//   bar 3 pixel 18 = gid 71  (very top)
// Motion recipes treat the rig as this single 72-pixel line.
inline constexpr int kNumLeds      = kNumBars * kPixelsPerBar;           // 72
inline constexpr int kBarChannels  = kNumLeds * 3;                       // 216

// Total reserved DMX footprint (spots + bars). Frame buffers are sized to this
// and indexed by (DMX channel - 1), so the spot channels 1-12 (indices 0-11)
// are carried through as zeros and the bars occupy indices 12..227.
// bar 3 pixel 18 = DMX 228 is the last reserved channel.
inline constexpr int kRigChannels  = kBarChannelBase - 1 + kBarChannels; // 228
static_assert(kRigChannels == 228);
static_assert(kRigChannels <= kDmxUniverseSize);

struct BarFixture
{
    int dmxStart;  // 1-based DMX channel of pixel 1's red component
    int pixels;    // always kPixelsPerBar

    constexpr std::array<int, 3> channelsFor(int pixel) const noexcept
    {
        const int base = dmxStart + (pixel - 1) * 3;
        return { base, base + 1, base + 2 };
    }
};

// bar N pixel-1 red = kBarChannelBase + N * kPixelsPerBar * 3  →  13 / 67 / 121 / 175.
inline constexpr std::array<BarFixture, kNumBars> kBars {{
    { kBarChannelBase + 0 * kPixelsPerBar * 3, kPixelsPerBar },
    { kBarChannelBase + 1 * kPixelsPerBar * 3, kPixelsPerBar },
    { kBarChannelBase + 2 * kPixelsPerBar * 3, kPixelsPerBar },
    { kBarChannelBase + 3 * kPixelsPerBar * 3, kPixelsPerBar },
}};

// Two RGBW spots pinned at the bottom of the universe. Reserved but unused by
// this app's recipes — kept for patch parity with hitnotedmx and drawn (dark)
// in the visualiser so the grid layout matches.
struct SpotFixture
{
    int dmxStart;

    constexpr int dimmer() const noexcept { return dmxStart + 0; }
    constexpr int red()    const noexcept { return dmxStart + 1; }
    constexpr int green()  const noexcept { return dmxStart + 2; }
    constexpr int blue()   const noexcept { return dmxStart + 3; }
    constexpr int white()  const noexcept { return dmxStart + 4; }
    constexpr int strobe() const noexcept { return dmxStart + 5; }
};

inline constexpr std::array<SpotFixture, kNumSpots> kSpots {{
    { 1 },
    { 7 },
}};

// Global 0-based LED index for (barIdx 0..3, pixel 1..18), bottom→top.
inline constexpr int gidFor(int barIdx, int pixel) noexcept
{
    return barIdx * kPixelsPerBar + (pixel - 1);
}

}  // namespace hitlaunchdmx
