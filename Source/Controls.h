#pragma once

#include <array>
#include <cmath>
#include <juce_core/juce_core.h>

namespace hitlaunchdmx
{

// ── The parameter controller: Novation Launch Control XL, Factory template 1 ──
//
// THE ONE EDITABLE MAPPING TABLE. Every continuous parameter and every button
// of this app lives at a fixed XL position; the engine, the hardware decoder
// (Surfaces.cpp) and the on-screen XL panel all read the tables below, so
// moving a function to another control is a one-line change here.
//
// Layout (8 channel strips, left → right):
//   knob row 0 "Send A"  → params  0.. 7
//   knob row 1 "Send B"  → params  8..15
//   knob row 2 "Pan"     → params 16..23
//   faders               → params 24..31  (per-bar dims, bar 1..7)
//   button row 0 "Track Focus"   → buttons 0..7   (bar bumps 1-7)
//   button row 1 "Track Control" → buttons 8..15  (tap, restart, reverse, fades, freeze, flip)
//
// Strips 1-7 are the bars (fader = bar dim, top button = bar bump). Strip 8 is
// the MASTER column: Density · Spot dim · Strobe Hz · Master (LED) dim fader ·
// Blackout (top) · Strobe (bottom).
//
// Params are stored normalised (0..1, = CC / 127); the helpers below turn them
// into engine values. The engine feeds most of them to hitnotedmx's computeDmx
// as the VELOCITY of a held note — the exact meaning hitnotedmx gives that
// velocity — so a knob here behaves like the same note velocity there.

inline constexpr int kNumStrips  = 8;
inline constexpr int kNumKnobRows = 3;
inline constexpr int kNumParams  = kNumStrips * (kNumKnobRows + 1);   // 24 knobs + 8 faders
inline constexpr int kFaderParam0 = kNumStrips * kNumKnobRows;        // 24
inline constexpr int kNumButtons = kNumStrips * 2;                    // 16

enum class P : int
{
    // Send A — tempo & motion
    Bpm = 0, RelSpeed, ChaseTail, BreatheDepth, WildDiv, ColorSpeed, Spread, Density,
    // Send B — accents & transitions
    BumpRelease = 8, BumpLevel, Crossfade, ColorFade, SpareB5, BlackGlide, SpareB7, SpotMaster,
    // Pan — spare except the master column's strobe rate
    SpareP1 = 16, SpareP2, SpareP3, SpareP4, SpareP5, SpareP6, SpareP7, StrobeRate,
    // Faders — per-bar dims 1..7, then the master (LED) dim
    BarDim1 = 24, LedMaster = 31,
};

inline constexpr int kNumBarFaders = 7;   // faders 1..7 = bar dims; fader 8 = master

inline constexpr int idx (P p) noexcept { return static_cast<int> (p); }

struct ParamDef
{
    const char* name;      // short label (on-screen knob caption)
    float       deflt;     // normalised default
    bool        used;      // false = spare (drawn greyed)
};

inline constexpr std::array<ParamDef, kNumParams> kParamDefs {{
    // Send A
    { "BPM",        0.5f,         true  },   // 60..180 → 120
    { "Rel speed",  0.5f,         true  },   // ¼ ½ 1 2 4 → 1×
    { "Chase tail", 0.5f,         true  },
    { "Breathe",    1.0f,         true  },   // glow-mask depth: 127 = untouched
    { "Wild div",   0.5f,         true  },
    { "Multi spd",  0.5f,         true  },
    { "Spread",     0.0f,         true  },   // 0 = off
    { "Density",    1.0f,         true  },
    // Send B
    { "Release",    0.25f,        true  },
    { "Bump lvl",   1.0f,         true  },
    { "Crossfade",  0.0f,         true  },   // 0 = instant
    { "Col fade",   0.0f,         true  },   // 0 = instant colour change
    { "-",          0.0f,         false },
    { "Blk glide",  0.0f,         true  },   // 0 = instant to/from black
    { "-",          0.0f,         false },
    { "Spot dim",   1.0f,         true  },   // master column
    // Pan
    { "-",          0.0f,         false },
    { "-",          0.0f,         false },
    { "-",          0.0f,         false },
    { "-",          0.0f,         false },
    { "-",          0.0f,         false },
    { "-",          0.0f,         false },
    { "-",          0.0f,         false },
    { "Strobe Hz",  0.5f,         true  },   // master column
    // Faders
    { "Bar 1", 1.0f, true }, { "Bar 2", 1.0f, true }, { "Bar 3", 1.0f, true }, { "Bar 4", 1.0f, true },
    { "Bar 5", 1.0f, true }, { "Bar 6", 1.0f, true }, { "Bar 7", 1.0f, true }, { "Master", 1.0f, true },
}};

// ── Buttons ──
enum class B : int
{
    BumpBar1 = 0, BumpBar2, BumpBar3, BumpBar4, BumpBar5, BumpBar6, BumpBar7, Blackout,  // Track Focus row
    Tap = 8, Restart, Reverse, FromBlack, ToBlack, Freeze, Flip, Strobe,                 // Track Control row
};

// Per-bar bumps (bars 1..7). Same flash as hitnotedmx's Bump — fires on the
// press, decays over the Release length, toward white or the held primary,
// at Bump lvl — but on one bar only. Done in this app (hitnotedmx's Bump is
// whole-rig); see LiveEngine::tick step 4.
inline constexpr int kNumBarBumps = 7;

inline constexpr int idx (B b) noexcept { return static_cast<int> (b); }

enum class ButtonKind { Momentary, Toggle, Trigger, Spare };

struct ButtonDef
{
    const char* name;
    ButtonKind  kind;
};

inline constexpr std::array<ButtonDef, kNumButtons> kButtonDefs {{
    { "Bump 1",     ButtonKind::Momentary },
    { "Bump 2",     ButtonKind::Momentary },
    { "Bump 3",     ButtonKind::Momentary },
    { "Bump 4",     ButtonKind::Momentary },
    { "Bump 5",     ButtonKind::Momentary },
    { "Bump 6",     ButtonKind::Momentary },
    { "Bump 7",     ButtonKind::Momentary },
    { "Blackout",   ButtonKind::Toggle    },   // same state as the Launchpad's Blackout
    { "Tap",        ButtonKind::Trigger   },
    { "Restart",    ButtonKind::Trigger   },
    { "Reverse",    ButtonKind::Toggle    },
    { "From black", ButtonKind::Toggle    },
    { "To black",   ButtonKind::Toggle    },
    { "Freeze",     ButtonKind::Toggle    },
    { "Flip",       ButtonKind::Toggle    },
    { "Strobe",     ButtonKind::Momentary },
}};

// ── Value helpers (normalised → engine units) ──

inline constexpr double kMinBpm = 60.0;
inline constexpr double kMaxBpm = 180.0;

inline double bpmFromNorm (float n) noexcept     { return kMinBpm + (kMaxBpm - kMinBpm) * static_cast<double> (n); }
inline float  normFromBpm (double bpm) noexcept
{
    return static_cast<float> (juce::jlimit (0.0, 1.0, (bpm - kMinBpm) / (kMaxBpm - kMinBpm)));
}

// Relative speed: 5 detented steps ¼× ½× 1× 2× 4× (power-of-two, so every
// cycle stays on the global beat grid).
inline constexpr int kRelSpeedSteps = 5;
inline int    relSpeedStep (float n) noexcept { return juce::jlimit (0, kRelSpeedSteps - 1, static_cast<int> (std::lround (n * (kRelSpeedSteps - 1)))); }
inline double relSpeedMult (int step) noexcept { return std::ldexp (1.0, step - 2); }

// Normalised → MIDI velocity 1..127 (a held note never has velocity 0).
inline int velFromNorm (float n) noexcept
{
    return juce::jlimit (1, 127, static_cast<int> (std::lround (n * 127.0f)));
}

// Human-readable value for the on-screen panel.
inline juce::String formatParam (int p, float n)
{
    switch (static_cast<P> (p))
    {
        case P::Bpm:       return juce::String (bpmFromNorm (n), 1);
        case P::RelSpeed:
        {
            static const char* names[] = { "1/4x", "1/2x", "1x", "2x", "4x" };
            return names[relSpeedStep (n)];
        }
        case P::Spread:    return n <= 0.0f ? juce::String ("off") : juce::String (velFromNorm (n));
        case P::Crossfade: return n <= 0.0f ? juce::String ("off") : juce::String (velFromNorm (n));
        case P::StrobeRate:
            return juce::String (1.0 + 19.0 * velFromNorm (n) / 127.0, 1) + "Hz";
        default: break;
    }
    if (! kParamDefs[static_cast<size_t> (p)].used)
        return {};
    return juce::String (juce::roundToInt (n * 100.0f)) + "%";
}

// ── Launch Control XL Factory template 1: MIDI numbers ──
// All messages on MIDI channel 9 (factory template 1). Knobs/faders send CC,
// the 16 strip buttons send Note On/Off.
namespace xl
{
inline constexpr int kChannel        = 9;
inline constexpr int kTemplateIndex  = 8;    // SysEx template id: user 0-7, factory 8-15

inline constexpr std::array<int, kNumStrips> kSendACc  { 13, 14, 15, 16, 17, 18, 19, 20 };
inline constexpr std::array<int, kNumStrips> kSendBCc  { 29, 30, 31, 32, 33, 34, 35, 36 };
inline constexpr std::array<int, kNumStrips> kPanCc    { 49, 50, 51, 52, 53, 54, 55, 56 };
inline constexpr std::array<int, kNumStrips> kFaderCc  { 77, 78, 79, 80, 81, 82, 83, 84 };
inline constexpr std::array<int, kNumStrips> kFocusNote   { 41, 42, 43, 44, 57, 58, 59, 60 };
inline constexpr std::array<int, kNumStrips> kControlNote { 73, 74, 75, 76, 89, 90, 91, 92 };

// CC number → param index (or -1).
inline int paramForCc (int cc) noexcept
{
    for (int i = 0; i < kNumStrips; ++i)
    {
        if (kSendACc[static_cast<size_t> (i)] == cc) return i;
        if (kSendBCc[static_cast<size_t> (i)] == cc) return kNumStrips + i;
        if (kPanCc  [static_cast<size_t> (i)] == cc) return 2 * kNumStrips + i;
        if (kFaderCc[static_cast<size_t> (i)] == cc) return kFaderParam0 + i;
    }
    return -1;
}

// Note number → button index (or -1).
inline int buttonForNote (int note) noexcept
{
    for (int i = 0; i < kNumStrips; ++i)
    {
        if (kFocusNote  [static_cast<size_t> (i)] == note) return i;
        if (kControlNote[static_cast<size_t> (i)] == note) return kNumStrips + i;
    }
    return -1;
}

// LED colour codes (velocity = 16 × green + red + 12 flags; 2-bit R/G).
inline constexpr int kLedOff    = 12;
inline constexpr int kLedRedLo  = 13;
inline constexpr int kLedRed    = 15;
inline constexpr int kLedAmberLo= 29;
inline constexpr int kLedAmber  = 63;
inline constexpr int kLedYellow = 62;
inline constexpr int kLedGreenLo= 28;
inline constexpr int kLedGreen  = 60;
}  // namespace xl

}  // namespace hitlaunchdmx
