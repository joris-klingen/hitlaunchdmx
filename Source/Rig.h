#pragma once

#include <array>
#include <cstdint>

#include "EnttecProDmx.h"

namespace hitlaunchdmx
{

// 4 RGB bars, 18 pixels each, bottom-up.
//   bar_0  : DMX   1– 54
//   bar_1  : DMX  55–108
//   bar_2  : DMX 109–162
//   bar_3  : DMX 163–216
//
// Bars are arranged in a physical ring (12 / 3 / 6 / 9 o'clock).
// Bar index arithmetic is cyclic: (barIdx + k) % kNumBars.

inline constexpr int kNumBars      = 4;
inline constexpr int kPixelsPerBar = 18;
inline constexpr int kRigChannels  = kNumBars * kPixelsPerBar * 3;  // 216
static_assert(kRigChannels == 216);
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

inline constexpr std::array<BarFixture, kNumBars> kBars {{
    {   1, kPixelsPerBar },
    {  55, kPixelsPerBar },
    { 109, kPixelsPerBar },
    { 163, kPixelsPerBar },
}};

// The 4 bars are stacked into ONE long vertical line of 72 LEDs.
// Global index (gid) runs 0..71 bottom→top:
//   bar 0 pixel 1 = gid 0   (very bottom)
//   bar 3 pixel 18 = gid 71 (very top)
// Motion recipes treat the rig as this single 72-pixel line.
inline constexpr int kNumLeds = kNumBars * kPixelsPerBar;  // 72

// Global 0-based LED index for (barIdx 0..3, pixel 1..18), bottom→top.
inline constexpr int gidFor(int barIdx, int pixel) noexcept
{
    return barIdx * kPixelsPerBar + (pixel - 1);
}

}  // namespace hitlaunchdmx
