#pragma once

#include <array>
#include <cstdint>

#include "Palette.h"

namespace hitlaunchdmx
{

// Parametric motion generators (implemented in SceneEngine.cpp).
enum class MotionType : uint8_t
{
    BREATHE,           // sinusoidal global dim — all bars together
    MOON_RISE,         // glow climbs vertically bottom→top
    DRIFT_BALL,        // soft Gaussian ball drifts around the ring
    RING_ROTATE,       // bar-wide highlight sweeps the ring
    RING_COMET,        // comet with tail travels the ring (bar-by-bar)
    VERT_CHASE,        // vertical comet, all bars in phase
    VERT_CHASE_STAGGER,// vertical comet, each bar offset by 90°
    COLOUR_WASH,       // slow cross-fade between color1 & color2 across bars
    PULSE_RING,        // radial sinusoidal pulse around the ring
    RAINBOW_RING,      // hue walks around the ring over time
    DIAGONAL_WIPE,     // vertical fill staggered per bar
};

struct Scene
{
    const char*  name;
    MotionType   motion;
    float        speed;       // wall-clock multiplier (cycles per second base)
    float        width;       // comet / ball half-width as fraction of track (0..1)
    float        brightness;  // master dim applied to output (0..1)
    PaletteColor color1;      // primary colour
    PaletteColor color2;      // secondary / gradient end colour
    int          direction;   // +1 or -1 (where applicable)
    uint8_t      padVelocity; // idle Launchpad LED colour byte
};

// ---------------------------------------------------------------------------
// 64 scene presets, indexed 0..63.
//
// Pad layout (Launchpad Mini MK3, Programmer mode):
//   note = row * 10 + col   (row 1=bottom..8=top, col 1=left..8=right)
//   scene index = (row - 1) * 8 + (col - 1)
//
//   Rows 1–4  (notes 11–48, scenes  0–31): 32 mild breathes
//   Rows 5–6  (notes 51–68, scenes 32–47): 16 medium chases
//   Rows 7–8  (notes 71–88, scenes 48–63): 16 wild chases
//
// To map a Launchpad note to a scene:
//   int row = note / 10 - 1;   // 0-based
//   int col = note % 10 - 1;   // 0-based
//   int idx = row * 8 + col;   // 0..63
// ---------------------------------------------------------------------------

// short namespace aliases to keep the table readable
namespace P = Pal;
namespace L = LpColor;

inline constexpr std::array<Scene, 64> kScenes {{

    // ── Row 1 (notes 11–18): mild breathes, warm palette ─────────────────
    { "Warm Breathe",       MotionType::BREATHE,    0.12f, 0.0f, 0.75f, P::WarmWhite, P::WarmWhite, +1, L::BreatheCat },
    { "Amber Breathe",      MotionType::BREATHE,    0.10f, 0.0f, 0.80f, P::Amber,     P::Amber,     +1, L::BreatheCat },
    { "Rose Breathe",       MotionType::BREATHE,    0.09f, 0.0f, 0.70f, P::SoftRose,  P::SoftRose,  +1, L::BreatheCat },
    { "Dim White Breathe",  MotionType::BREATHE,    0.08f, 0.0f, 0.60f, P::DimWhite,  P::DimWhite,  +1, L::BreatheCat },
    { "Lavender Breathe",   MotionType::BREATHE,    0.11f, 0.0f, 0.65f, P::Lavender,  P::Lavender,  +1, L::BreatheCat },
    { "Soft Blue Breathe",  MotionType::BREATHE,    0.10f, 0.0f, 0.60f, P::SoftBlue,  P::SoftBlue,  +1, L::BreatheCat },
    { "Orange Breathe",     MotionType::BREATHE,    0.13f, 0.0f, 0.75f, P::Orange,    P::Orange,    +1, L::BreatheCat },
    { "Mint Breathe",       MotionType::BREATHE,    0.09f, 0.0f, 0.60f, P::Mint,      P::Mint,      +1, L::BreatheCat },

    // ── Row 2 (notes 21–28): moon-rise + drift-ball ───────────────────────
    { "Amber Moon Rise",    MotionType::MOON_RISE,  0.07f, 0.0f, 0.80f, P::Amber,     P::Amber,     +1, L::BreatheCat },
    { "Warm Moon Rise",     MotionType::MOON_RISE,  0.06f, 0.0f, 0.75f, P::WarmWhite, P::WarmWhite, +1, L::BreatheCat },
    { "Blue Moon Rise",     MotionType::MOON_RISE,  0.06f, 0.0f, 0.65f, P::SoftBlue,  P::SoftBlue,  +1, L::BreatheCat },
    { "Rose Moon Rise",     MotionType::MOON_RISE,  0.07f, 0.0f, 0.70f, P::SoftRose,  P::SoftRose,  +1, L::BreatheCat },
    { "Amber Drift Ball",   MotionType::DRIFT_BALL, 0.05f, 0.25f,0.80f, P::Amber,     P::Amber,     +1, L::BreatheCat },
    { "Warm Drift Ball",    MotionType::DRIFT_BALL, 0.04f, 0.30f,0.75f, P::WarmWhite, P::WarmWhite, +1, L::BreatheCat },
    { "Rose Drift Ball",    MotionType::DRIFT_BALL, 0.05f, 0.25f,0.70f, P::SoftRose,  P::SoftRose,  -1, L::BreatheCat },
    { "Lavender Drift Ball",MotionType::DRIFT_BALL, 0.04f, 0.28f,0.65f, P::Lavender,  P::Lavender,  +1, L::BreatheCat },

    // ── Row 3 (notes 31–38): ring-rotate + colour-wash ───────────────────
    { "Warm Ring Rotate",   MotionType::RING_ROTATE,0.08f, 0.0f, 0.80f, P::WarmWhite, P::WarmWhite, +1, L::BreatheCat },
    { "Amber Ring Rotate",  MotionType::RING_ROTATE,0.07f, 0.0f, 0.75f, P::Amber,     P::Amber,     +1, L::BreatheCat },
    { "Rose Ring Rotate",   MotionType::RING_ROTATE,0.06f, 0.0f, 0.70f, P::SoftRose,  P::SoftRose,  -1, L::BreatheCat },
    { "Lavender Ring Rotate",MotionType::RING_ROTATE,0.07f,0.0f, 0.65f, P::Lavender,  P::Lavender,  +1, L::BreatheCat },
    { "Amber→Rose Wash",    MotionType::COLOUR_WASH,0.05f, 0.0f, 0.75f, P::Amber,     P::SoftRose,  +1, L::BreatheCat },
    { "Warm→Lavender Wash", MotionType::COLOUR_WASH,0.04f, 0.0f, 0.70f, P::WarmWhite, P::Lavender,  +1, L::BreatheCat },
    { "Blue→Teal Wash",     MotionType::COLOUR_WASH,0.05f, 0.0f, 0.65f, P::SoftBlue,  P::Teal,      +1, L::BreatheCat },
    { "Rose→Orange Wash",   MotionType::COLOUR_WASH,0.04f, 0.0f, 0.72f, P::SoftRose,  P::Orange,    -1, L::BreatheCat },

    // ── Row 4 (notes 41–48): pulse-ring + breathe variants ───────────────
    { "Warm Pulse Ring",    MotionType::PULSE_RING, 0.10f, 0.0f, 0.75f, P::WarmWhite, P::WarmWhite, +1, L::BreatheCat },
    { "Amber Pulse Ring",   MotionType::PULSE_RING, 0.09f, 0.0f, 0.80f, P::Amber,     P::Amber,     +1, L::BreatheCat },
    { "Lavender Pulse Ring",MotionType::PULSE_RING, 0.08f, 0.0f, 0.65f, P::Lavender,  P::Lavender,  +1, L::BreatheCat },
    { "Rose Pulse Ring",    MotionType::PULSE_RING, 0.10f, 0.0f, 0.70f, P::SoftRose,  P::SoftRose,  -1, L::BreatheCat },
    { "Teal Breathe",       MotionType::BREATHE,    0.09f, 0.0f, 0.60f, P::Teal,      P::Teal,      +1, L::BreatheCat },
    { "Crimson Breathe",    MotionType::BREATHE,    0.11f, 0.0f, 0.65f, P::Crimson,   P::Crimson,   +1, L::BreatheCat },
    { "Cool White Breathe", MotionType::BREATHE,    0.08f, 0.0f, 0.60f, P::CoolWhite, P::CoolWhite, +1, L::BreatheCat },
    { "Violet Breathe",     MotionType::BREATHE,    0.10f, 0.0f, 0.60f, P::Violet,    P::Violet,    +1, L::BreatheCat },

    // ── Row 5 (notes 51–58): medium ring comets + vertical chases ────────
    { "Amber Ring Comet",   MotionType::RING_COMET, 0.25f, 0.35f,0.90f, P::Amber,     P::Amber,     +1, L::MedCat },
    { "Warm Ring Comet CW", MotionType::RING_COMET, 0.20f, 0.30f,0.85f, P::WarmWhite, P::WarmWhite, +1, L::MedCat },
    { "Blue Ring Comet CCW",MotionType::RING_COMET, 0.22f, 0.30f,0.80f, P::SoftBlue,  P::SoftBlue,  -1, L::MedCat },
    { "Rose Ring Comet",    MotionType::RING_COMET, 0.25f, 0.32f,0.85f, P::SoftRose,  P::SoftRose,  +1, L::MedCat },
    { "Amber Vert Chase Up",MotionType::VERT_CHASE, 0.30f, 0.30f,0.90f, P::Amber,     P::Amber,     +1, L::MedCat },
    { "Warm Vert Chase Dn", MotionType::VERT_CHASE, 0.28f, 0.28f,0.85f, P::WarmWhite, P::WarmWhite, -1, L::MedCat },
    { "Teal Vert Chase Up", MotionType::VERT_CHASE, 0.32f, 0.28f,0.80f, P::Teal,      P::Teal,      +1, L::MedCat },
    { "Rose Vert Chase Dn", MotionType::VERT_CHASE, 0.30f, 0.30f,0.85f, P::SoftRose,  P::SoftRose,  -1, L::MedCat },

    // ── Row 6 (notes 61–68): medium stagger + colour-alt chases ─────────
    { "Amber Stagger Up",   MotionType::VERT_CHASE_STAGGER, 0.28f,0.28f,0.85f, P::Amber,    P::Amber,    +1, L::MedCat },
    { "Warm Stagger Dn",    MotionType::VERT_CHASE_STAGGER, 0.25f,0.25f,0.80f, P::WarmWhite,P::WarmWhite,-1, L::MedCat },
    { "Teal Stagger Up",    MotionType::VERT_CHASE_STAGGER, 0.30f,0.28f,0.80f, P::Teal,     P::Teal,     +1, L::MedCat },
    { "Lavender Stagger Dn",MotionType::VERT_CHASE_STAGGER, 0.26f,0.26f,0.75f, P::Lavender, P::Lavender, -1, L::MedCat },
    { "Diagonal Wipe Up",   MotionType::DIAGONAL_WIPE, 0.25f,0.0f,0.90f, P::Amber,     P::WarmWhite, +1, L::MedCat },
    { "Diagonal Wipe Dn",   MotionType::DIAGONAL_WIPE, 0.22f,0.0f,0.85f, P::SoftRose,  P::Lavender,  -1, L::MedCat },
    { "Amber→Teal Diag",    MotionType::DIAGONAL_WIPE, 0.28f,0.0f,0.85f, P::Amber,     P::Teal,      +1, L::MedCat },
    { "Blue Diag Dn",       MotionType::DIAGONAL_WIPE, 0.24f,0.0f,0.80f, P::SoftBlue,  P::Lavender,  -1, L::MedCat },

    // ── Row 7 (notes 71–78): wild fast ring comets + fast vert ──────────
    { "Orange Ring Fast CW",MotionType::RING_COMET, 0.70f, 0.25f,1.00f, P::OrangeRed, P::Yellow,    +1, L::WildCat },
    { "Cyan Ring Fast CCW", MotionType::RING_COMET, 0.65f, 0.25f,1.00f, P::Cyan,      P::Blue,      -1, L::WildCat },
    { "Magenta Ring Fast",  MotionType::RING_COMET, 0.75f, 0.22f,1.00f, P::Magenta,   P::Violet,    +1, L::WildCat },
    { "Green Ring Fast CCW",MotionType::RING_COMET, 0.68f, 0.22f,1.00f, P::Green,     P::Lime,      -1, L::WildCat },
    { "Red Vert Chase Fast",MotionType::VERT_CHASE, 0.80f, 0.22f,1.00f, P::Red,       P::OrangeRed, +1, L::WildCat },
    { "Cyan Vert Fast Dn",  MotionType::VERT_CHASE, 0.75f, 0.22f,1.00f, P::Cyan,      P::Blue,      -1, L::WildCat },
    { "Magenta Stagger Fast",MotionType::VERT_CHASE_STAGGER,0.80f,0.20f,1.00f,P::Magenta,P::Pink,   +1, L::WildCat },
    { "Green Stagger Fast", MotionType::VERT_CHASE_STAGGER, 0.75f,0.20f,1.00f,P::Green,  P::Lime,   +1, L::WildCat },

    // ── Row 8 (notes 81–88): wild rainbow + high-contrast ───────────────
    { "Rainbow Ring CW",    MotionType::RAINBOW_RING,0.30f,0.0f, 1.00f, P::Red,       P::Blue,     +1, L::WildCat },
    { "Rainbow Ring CCW",   MotionType::RAINBOW_RING,0.28f,0.0f, 1.00f, P::Blue,      P::Red,      -1, L::WildCat },
    { "Rainbow Vert Fast",  MotionType::RAINBOW_RING,0.40f,0.0f, 1.00f, P::Green,     P::Magenta,  +1, L::WildCat },
    { "Rainbow Pulse",      MotionType::RAINBOW_RING,0.20f,0.0f, 0.90f, P::Orange,    P::Cyan,     -1, L::WildCat },
    { "Orange→Cyan Wash Fast",MotionType::COLOUR_WASH,0.50f,0.0f,1.00f,P::OrangeRed, P::Cyan,     +1, L::WildCat },
    { "Magenta→Green Wash", MotionType::COLOUR_WASH, 0.45f,0.0f, 1.00f, P::Magenta,   P::Green,    +1, L::WildCat },
    { "Red Pulse Ring Fast",MotionType::PULSE_RING,  0.50f,0.0f, 1.00f, P::Red,       P::OrangeRed,+1, L::WildCat },
    { "Cyan Pulse Ring Fast",MotionType::PULSE_RING, 0.48f,0.0f, 1.00f, P::Cyan,      P::Blue,     +1, L::WildCat },
}};

// Convert a Launchpad Mini MK3 Programmer-mode note to a scene index.
// Returns -1 if the note is not a grid pad (11–88, col digit 1–8).
inline constexpr int noteToSceneIndex(int note) noexcept
{
    const int row = note / 10 - 1;  // 0-based, 0 = bottom
    const int col = note % 10 - 1;  // 0-based
    if (row < 0 || row > 7 || col < 0 || col > 7)
        return -1;
    return row * 8 + col;
}

}  // namespace hitlaunchdmx
