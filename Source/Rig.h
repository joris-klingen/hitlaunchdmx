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

// Ring angles for the visualiser (radians, clockwise from top).
//   bar 0 → 12 o'clock (top)
//   bar 1 →  3 o'clock (right)
//   bar 2 →  6 o'clock (bottom)
//   bar 3 →  9 o'clock (left)
inline constexpr float kBarAngle[kNumBars] = {
    0.0f,                                // 12 o'clock
    3.14159265f / 2.0f,                  //  3 o'clock
    3.14159265f,                         //  6 o'clock
    3.0f * 3.14159265f / 2.0f,           //  9 o'clock
};

}  // namespace hitlaunchdmx
