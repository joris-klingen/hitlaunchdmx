#pragma once

#include <array>
#include <initializer_list>

#include "Palette.h"   // hitnotedmx
#include "Recipes.h"   // hitnotedmx

namespace hitlaunchdmx
{

// ── Auto modes (Launchpad top row, buttons 5-8) ─────────────────────────────
//
// Each level loops through its programs, switching on the GLOBAL bar grid
// (every `barsPerProgram` bars, counted from when the level was started).
// A program is just a set of hitnotedmx notes, like a stored look — the XL
// knobs still give them their velocities (tail, depth, speed, …). Spots are
// never touched by auto (they stay on their own top-row buttons).
//
// Wildness per level:
//   • transitions: `xfadeVel` > 0 holds hitnotedmx's Crossfade note around each
//     change (127 = one-bar glide); `colourVel` caps the palette velocity so
//     colours glide (lower = slower, 127 = instant). The XL Crossfade / Col
//     fade knobs still win when they ask for MORE smoothing.
//   • the change rate (`barsPerProgram`) and the program content.
// Auto adds nothing on top of the programs (no automatic bumps) — accents
// stay on the XL bump buttons.
//
// THE EDITABLE TABLE: change, add or reorder programs freely (up to
// kMaxPrograms per level, up to kMaxNotes notes per program).

inline constexpr int kNumAutoLevels = 4;
inline constexpr int kMaxPrograms   = 8;
inline constexpr int kMaxNotes      = 4;

struct AutoProgram
{
    // Built from a note list; unused slots are -1. (A plain aggregate would
    // zero-fill them — and note 0 is hitnotedmx's Blackout.)
    AutoProgram (std::initializer_list<int> ns) noexcept
    {
        notes.fill (-1);
        int i = 0;
        for (const int n : ns)
            if (i < kMaxNotes) notes[static_cast<size_t> (i++)] = n;
    }

    std::array<int, kMaxNotes> notes {};   // hitnotedmx pitches, -1 = unused
};

struct AutoLevel
{
    const char* name;
    int  barsPerProgram;
    int  xfadeVel;          // 0 = cut
    int  colourVel;         // palette velocity cap (127 = instant)
    int  numPrograms;
    std::array<AutoProgram, kMaxPrograms> programs;
};

namespace autonotes
{
using namespace hitnotedmx;
// Chases
inline constexpr int Chase = kChasesStart + 0,  Ripple = kChasesStart + 1,  PingPong = kChasesStart + 2,
                     Diag  = kChasesStart + 3,  Radar  = kChasesStart + 4,  Snake    = kChasesStart + 5,
                     Theater = kChasesStart + 6, Spiral = kChasesStart + 7, Waves    = kChasesStart + 8,
                     Expand = kChasesStart + 9, Contract = kChasesStart + 10, ChaseH = kChasesStart + 11;
// Breathes
inline constexpr int Tide = kBreathesStart + 0, Sine = kBreathesStart + 1, Pond = kBreathesStart + 2,
                     Gyre = kBreathesStart + 3, Bloom = kBreathesStart + 4, Halo = kBreathesStart + 5,
                     MoonRise = kBreathesStart + 6, SoftBall = kBreathesStart + 7, Drift = kBreathesStart + 8,
                     Aurora = kBreathesStart + 9, Shimmer = kBreathesStart + 10, Glow = kBreathesStart + 11;
// Wild (no Strobe in auto — that stays a deliberate button press)
inline constexpr int Sparkle = kWildStart + 1, SparkleFew = kWildStart + 2, Lightning = kWildStart + 3,
                     Glitch = kWildStart + 4, Fountain = kWildStart + 5, Rain = kWildStart + 6,
                     Waterfalls = kWildStart + 7, Bounce = kWildStart + 8, FastBall = kWildStart + 9,
                     Pong = kWildStart + 10, Burst = kWildStart + 11;
// Multicolor
inline constexpr int Rainbow = kColorDynStart + 0, Comet = kColorDynStart + 1, Fire = kColorDynStart + 4,
                     Embers = kColorDynStart + 5, Lava = kColorDynStart + 7, Ocean = kColorDynStart + 9,
                     Forest = kColorDynStart + 10, Sunset = kColorDynStart + 12, Twilight = kColorDynStart + 13,
                     Borealis = kColorDynStart + 14, NightSky = kColorDynStart + 15, Galaxy = kColorDynStart + 16,
                     Plasma = kColorDynStart + 19, Police = kColorDynStart + 20, Disco = kColorDynStart + 21,
                     Velvet = kColorDynStart + 22;
// Primary colours
inline constexpr int Red = kPrimaryPaletteStart + 1, Orange = kPrimaryPaletteStart + 3,
                     Amber = kPrimaryPaletteStart + 4, Yellow = kPrimaryPaletteStart + 5,
                     Lime = kPrimaryPaletteStart + 6, Teal = kPrimaryPaletteStart + 9,
                     Cyan = kPrimaryPaletteStart + 10, Blue = kPrimaryPaletteStart + 12,
                     Royal = kPrimaryPaletteStart + 13, Violet = kPrimaryPaletteStart + 15,
                     Magenta = kPrimaryPaletteStart + 17, Pink = kPrimaryPaletteStart + 18,
                     HotPink = kPrimaryPaletteStart + 19, WarmWhite = kPrimaryPaletteStart + 21,
                     CoolWhite = kPrimaryPaletteStart + 22;
}  // namespace autonotes

inline const std::array<AutoLevel, kNumAutoLevels>& autoLevels()
{
    using namespace autonotes;
    static const std::array<AutoLevel, kNumAutoLevels> levels {{
        { "Mellow", 8, 127, 20, 8, {{
            AutoProgram { Tide, Blue },          AutoProgram { MoonRise, Amber },     AutoProgram { Ocean },               AutoProgram { SoftBall, Violet },
            AutoProgram { Aurora, Teal },        AutoProgram { Borealis },            AutoProgram { Drift, WarmWhite },    AutoProgram { Twilight },
        }}},
        { "Groove", 4, 100, 70, 8, {{
            AutoProgram { Waves, Cyan },         AutoProgram { Bloom, Magenta },      AutoProgram { Expand, Orange },      AutoProgram { Sunset },
            AutoProgram { Ripple, Royal },       AutoProgram { Halo, Pink },          AutoProgram { Theater, Amber },      AutoProgram { Lava },
        }}},
        { "Party", 2, 0, 127, 8, {{
            AutoProgram { Chase, Red },          AutoProgram { Bounce, Cyan },        AutoProgram { Fire, ChaseH },        AutoProgram { Spiral, Lime },
            AutoProgram { Rain, Blue },          AutoProgram { Plasma },              AutoProgram { PingPong, HotPink },   AutoProgram { Fountain, Yellow },
        }}},
        { "Disco", 1, 0, 127, 8, {{
            AutoProgram { Disco },               AutoProgram { Sparkle, Magenta },    AutoProgram { Rainbow, Chase },      AutoProgram { FastBall, Cyan },
            AutoProgram { Police },              AutoProgram { Glitch, Yellow },      AutoProgram { Pong, HotPink },       AutoProgram { Lightning, CoolWhite },
        }}},
    }};
    return levels;
}

}  // namespace hitlaunchdmx
