#pragma once

#include <array>
#include <cstdint>

#include "Palette.h"

namespace hitlaunchdmx
{

// Parametric motion generators (implemented in SceneEngine.cpp).
// All operate over the single 72-LED vertical line (gid 0..71), except the
// QUAD_* recipes which run independently within each 18-LED bar.
enum class MotionType : uint8_t
{
    MOON_RISE,       // glow climbs the full line bottom -> top
    DRIFT_BALL,      // soft Gaussian ball drifts up/down the full line
    SWEEP,           // wide highlight sweeps the line, wrapping
    COMET,           // single comet with tail, wrapping
    MULTI_COMET,     // `reps` comets evenly spaced on the wrapping line
    PING_PONG,       // comet bounces up the full line then back down
    QUAD_CHASE,      // comet chase within EACH bar (4 at once)
    QUAD_PING_PONG,  // ping-pong within each bar (mirrored on alt bars)
    ODD_EVEN,        // shimmer crossfading between odd & even LEDs
    COLOUR_WASH,     // travelling colour wave color1<->color2 (`reps` cycles)
    PULSE,           // travelling brightness wave (`reps` cycles)
    RAINBOW,         // hue scrolls along the line (`reps` rainbows)
    WIPE,            // fill sweeps along the line, colour crossfades
};

struct Scene
{
    const char*  name;
    MotionType   motion;
    float        speed;       // wall-clock motion rate
    float        width;       // comet tail / ball size as fraction of track
    float        brightness;  // master dim applied to output (0..1)
    PaletteColor color1;      // primary colour (also the Launchpad pad colour)
    PaletteColor color2;      // secondary / gradient end colour
    int          direction;   // +1 or -1
    int          reps;        // comet count / spatial cycles (1 = single)
};

// ---------------------------------------------------------------------------
// Scene presets, organised into pages of 64 (one full 8x8 grid each).
//
// Pad layout (Launchpad Mini MK3, Programmer mode):
//   note = row * 10 + col   (row 1=bottom..8=top, col 1=left..8=right)
//   in-page scene index = (row - 1) * 8 + (col - 1)
//   flat scene index     = page * 64 + in-page index
//
// kScenes is flat: indices 0..63 = page 1, 64..127 = page 2.
// Within each page: calm at the bottom (row 1) -> wildest at the top (row 8).
//   Rows 1-2 : ambient glows, sweeps, gentle pulses / odd-even
//   Rows 3-4 : colour washes + wipes
//   Rows 5-6 : comets, multi-comets, quad chases & ping-pongs
//   Rows 7-8 : wild fast multi-comets, odd/even, rainbows, fast washes
//
// Page 1 leans warm / red; page 2 leans cool / jewel tones.
// ---------------------------------------------------------------------------

inline constexpr int kNumPages      = 2;
inline constexpr int kScenesPerPage = 64;
inline constexpr int kTotalScenes   = kNumPages * kScenesPerPage;

namespace P = Pal;

inline constexpr std::array<Scene, kTotalScenes> kScenes {{

    // ======================================================================
    // PAGE 1  (flat 0..63) — warm / red emphasis
    // ======================================================================

    // ── Row 1 (notes 11-18): ambient glows ────────────────────────────────
    { "Amber Moon",      MotionType::MOON_RISE,  0.07f, 0.0f,  0.85f, P::Amber,     P::Amber,     +1, 1 },
    { "Warm Moon",       MotionType::MOON_RISE,  0.06f, 0.0f,  0.80f, P::WarmWhite, P::WarmWhite, +1, 1 },
    { "Rose Moon",       MotionType::MOON_RISE,  0.07f, 0.0f,  0.75f, P::SoftRose,  P::SoftRose,  -1, 1 },
    { "Blue Moon",       MotionType::MOON_RISE,  0.06f, 0.0f,  0.70f, P::SoftBlue,  P::SoftBlue,  +1, 1 },
    { "Amber Ball",      MotionType::DRIFT_BALL, 0.05f, 0.30f, 0.85f, P::Amber,     P::Amber,     +1, 1 },
    { "Warm Ball",       MotionType::DRIFT_BALL, 0.04f, 0.32f, 0.80f, P::WarmWhite, P::WarmWhite, +1, 1 },
    { "Rose Ball",       MotionType::DRIFT_BALL, 0.05f, 0.30f, 0.75f, P::SoftRose,  P::SoftRose,  -1, 1 },
    { "Lav Ball",        MotionType::DRIFT_BALL, 0.04f, 0.30f, 0.70f, P::Lavender,  P::Lavender,  +1, 1 },

    // ── Row 2 (notes 21-28): sweeps + gentle pulses / shimmer ─────────────
    { "Amber Sweep",     MotionType::SWEEP,      0.10f, 0.0f,  0.85f, P::Amber,     P::Amber,     +1, 1 },
    { "Warm Sweep",      MotionType::SWEEP,      0.08f, 0.0f,  0.80f, P::WarmWhite, P::WarmWhite, -1, 1 },
    { "Teal Sweep",      MotionType::SWEEP,      0.10f, 0.0f,  0.75f, P::Teal,      P::Teal,      +1, 1 },
    { "Violet Sweep",    MotionType::SWEEP,      0.09f, 0.0f,  0.75f, P::Violet,    P::Violet,    -1, 1 },
    { "Amber Pulse",     MotionType::PULSE,      0.10f, 0.0f,  0.80f, P::Amber,     P::Amber,     +1, 1 },
    { "Rose Pulse x2",   MotionType::PULSE,      0.10f, 0.0f,  0.75f, P::SoftRose,  P::SoftRose,  +1, 2 },
    { "Blue Pulse x3",   MotionType::PULSE,      0.12f, 0.0f,  0.70f, P::SoftBlue,  P::SoftBlue,  -1, 3 },
    { "Warm Shimmer",    MotionType::ODD_EVEN,   0.20f, 0.0f,  0.75f, P::WarmWhite, P::WarmWhite, +1, 1 },

    // ── Row 3 (notes 31-38): colour washes (red tones first) ──────────────
    { "Red Wash",        MotionType::COLOUR_WASH,0.06f, 0.0f,  0.90f, P::Red,       P::Crimson,   +1, 1 },
    { "Crimson Wash",    MotionType::COLOUR_WASH,0.05f, 0.0f,  0.85f, P::Crimson,   P::OrangeRed, +1, 1 },
    { "Red-Orange Wash", MotionType::COLOUR_WASH,0.07f, 0.0f,  0.90f, P::Red,       P::Orange,    -1, 1 },
    { "Red-Pink Wash",   MotionType::COLOUR_WASH,0.06f, 0.0f,  0.85f, P::Red,       P::HotPink,   +1, 1 },
    { "Amber-Rose Wash", MotionType::COLOUR_WASH,0.05f, 0.0f,  0.85f, P::Amber,     P::SoftRose,  +1, 1 },
    { "Warm-Lav Wash",   MotionType::COLOUR_WASH,0.04f, 0.0f,  0.80f, P::WarmWhite, P::Lavender,  +1, 1 },
    { "Blue-Teal Wash",  MotionType::COLOUR_WASH,0.05f, 0.0f,  0.75f, P::SoftBlue,  P::Teal,      +1, 1 },
    { "Rose-Orange Wash",MotionType::COLOUR_WASH,0.05f, 0.0f,  0.85f, P::SoftRose,  P::Orange,    -1, 1 },

    // ── Row 4 (notes 41-48): multi-cycle washes + wipes ───────────────────
    { "Red Wash x2",     MotionType::COLOUR_WASH,0.08f, 0.0f,  0.90f, P::Red,       P::Crimson,   +1, 2 },
    { "Crim-Mag Wash x2",MotionType::COLOUR_WASH,0.07f, 0.0f,  0.85f, P::Crimson,   P::Magenta,   -1, 2 },
    { "Orange-Red x3",   MotionType::COLOUR_WASH,0.09f, 0.0f,  0.90f, P::Orange,    P::Red,       +1, 3 },
    { "Teal-Blue Wash",  MotionType::COLOUR_WASH,0.06f, 0.0f,  0.80f, P::Teal,      P::Blue,      +1, 1 },
    { "Amber Wipe",      MotionType::WIPE,       0.12f, 0.0f,  0.90f, P::Amber,     P::WarmWhite, +1, 1 },
    { "Red Wipe",        MotionType::WIPE,       0.12f, 0.0f,  0.90f, P::Red,       P::Orange,    +1, 1 },
    { "Blue Wipe Dn",    MotionType::WIPE,       0.10f, 0.0f,  0.80f, P::SoftBlue,  P::Lavender,  -1, 1 },
    { "Rose Wipe",       MotionType::WIPE,       0.11f, 0.0f,  0.85f, P::SoftRose,  P::HotPink,   +1, 1 },

    // ── Row 5 (notes 51-58): comets & multi-comets ────────────────────────
    { "Amber Comet",     MotionType::COMET,      0.25f, 0.30f, 0.95f, P::Amber,     P::Amber,     +1, 1 },
    { "Warm Comet Dn",   MotionType::COMET,      0.22f, 0.30f, 0.90f, P::WarmWhite, P::WarmWhite, -1, 1 },
    { "Red Comet",       MotionType::COMET,      0.28f, 0.28f, 1.00f, P::Red,       P::Red,       +1, 1 },
    { "Teal Comet Dn",   MotionType::COMET,      0.25f, 0.28f, 0.85f, P::Teal,      P::Teal,      -1, 1 },
    { "Amber 2-Comet",   MotionType::MULTI_COMET,0.22f, 0.25f, 0.95f, P::Amber,     P::Amber,     +1, 2 },
    { "Rose 2-Comet",    MotionType::MULTI_COMET,0.22f, 0.25f, 0.90f, P::SoftRose,  P::SoftRose,  -1, 2 },
    { "Sky 3-Comet",     MotionType::MULTI_COMET,0.25f, 0.20f, 0.85f, P::Sky,       P::Sky,       +1, 3 },
    { "Red 3-Comet",     MotionType::MULTI_COMET,0.28f, 0.20f, 1.00f, P::Red,       P::Red,       +1, 3 },

    // ── Row 6 (notes 61-68): quad (per-bar) chases & ping-pongs ───────────
    { "Amber Quad Chase",MotionType::QUAD_CHASE, 0.30f, 0.40f, 0.95f, P::Amber,     P::Amber,     +1, 1 },
    { "Teal Quad Chase", MotionType::QUAD_CHASE, 0.28f, 0.40f, 0.85f, P::Teal,      P::Teal,      -1, 1 },
    { "Rose Quad Chase", MotionType::QUAD_CHASE, 0.30f, 0.35f, 0.90f, P::SoftRose,  P::SoftRose,  +1, 1 },
    { "Amber Quad PP",   MotionType::QUAD_PING_PONG,0.30f,0.45f,0.95f,P::Amber,     P::Amber,     +1, 1 },
    { "Cyan Quad PP",    MotionType::QUAD_PING_PONG,0.32f,0.45f,0.90f,P::Cyan,      P::Cyan,      +1, 1 },
    { "Warm Ping Pong",  MotionType::PING_PONG,  0.25f, 0.30f, 0.90f, P::WarmWhite, P::WarmWhite, +1, 1 },
    { "Red Ping Pong",   MotionType::PING_PONG,  0.30f, 0.28f, 1.00f, P::Red,       P::Red,       +1, 1 },
    { "Violet Ping Pong",MotionType::PING_PONG,  0.26f, 0.30f, 0.85f, P::Violet,    P::Violet,    -1, 1 },

    // ── Row 7 (notes 71-78): wild multi-comets, shimmer, fast washes ──────
    { "Orange 3-Comet F",MotionType::MULTI_COMET,0.55f, 0.18f, 1.00f, P::OrangeRed, P::OrangeRed, +1, 3 },
    { "Cyan 3-Comet F",  MotionType::MULTI_COMET,0.55f, 0.18f, 1.00f, P::Cyan,      P::Cyan,      -1, 3 },
    { "Magenta 4-Comet", MotionType::MULTI_COMET,0.50f, 0.16f, 1.00f, P::Magenta,   P::Magenta,   +1, 4 },
    { "Green 4-Comet",   MotionType::MULTI_COMET,0.50f, 0.16f, 1.00f, P::Green,     P::Green,     -1, 4 },
    { "Red Shimmer F",   MotionType::ODD_EVEN,   0.50f, 0.0f,  1.00f, P::Red,       P::Red,       +1, 1 },
    { "Cyan Shimmer F",  MotionType::ODD_EVEN,   0.50f, 0.0f,  1.00f, P::Cyan,      P::Cyan,      +1, 1 },
    { "Red Wash F x3",   MotionType::COLOUR_WASH,0.50f, 0.0f,  1.00f, P::Red,       P::OrangeRed, +1, 3 },
    { "Mag-Green F x2",  MotionType::COLOUR_WASH,0.45f, 0.0f,  1.00f, P::Magenta,   P::Green,     +1, 2 },

    // ── Row 8 (notes 81-88): wildest — rainbows & fast ────────────────────
    { "Rainbow Vert",    MotionType::RAINBOW,    0.30f, 0.0f,  1.00f, P::Red,       P::Blue,      +1, 1 },
    { "Rainbow Vert F",  MotionType::RAINBOW,    0.55f, 0.0f,  1.00f, P::Red,       P::Blue,      +1, 1 },
    { "Rainbow x2",      MotionType::RAINBOW,    0.40f, 0.0f,  1.00f, P::Red,       P::Blue,      +1, 2 },
    { "Rainbow x3 Dn",   MotionType::RAINBOW,    0.45f, 0.0f,  1.00f, P::Red,       P::Blue,      -1, 3 },
    { "Red Quad PP F",   MotionType::QUAD_PING_PONG,0.60f,0.35f,1.00f,P::Red,       P::Red,       +1, 1 },
    { "Cyan 4-Comet F",  MotionType::MULTI_COMET,0.70f, 0.14f, 1.00f, P::Cyan,      P::Cyan,      +1, 4 },
    { "Red Pulse F x4",  MotionType::PULSE,      0.60f, 0.0f,  1.00f, P::Red,       P::Red,       +1, 4 },
    { "Magenta 5-Comet", MotionType::MULTI_COMET,0.70f, 0.12f, 1.00f, P::Magenta,   P::Magenta,   +1, 5 },

    // ======================================================================
    // PAGE 2  (flat 64..127) — cool / jewel emphasis
    // ======================================================================

    // ── Row 1 (notes 11-18): cool ambient glows ───────────────────────────
    { "Teal Moon",       MotionType::MOON_RISE,  0.07f, 0.0f,  0.85f, P::Teal,      P::Teal,      +1, 1 },
    { "Green Moon",      MotionType::MOON_RISE,  0.06f, 0.0f,  0.80f, P::Green,     P::Green,     +1, 1 },
    { "Violet Moon",     MotionType::MOON_RISE,  0.07f, 0.0f,  0.75f, P::Violet,    P::Violet,    -1, 1 },
    { "Cyan Moon",       MotionType::MOON_RISE,  0.06f, 0.0f,  0.75f, P::Cyan,      P::Cyan,      +1, 1 },
    { "Teal Ball",       MotionType::DRIFT_BALL, 0.05f, 0.30f, 0.85f, P::Teal,      P::Teal,      +1, 1 },
    { "Green Ball",      MotionType::DRIFT_BALL, 0.04f, 0.32f, 0.80f, P::Green,     P::Green,     +1, 1 },
    { "Purple Ball",     MotionType::DRIFT_BALL, 0.05f, 0.30f, 0.75f, P::Purple,    P::Purple,    -1, 1 },
    { "Sky Ball",        MotionType::DRIFT_BALL, 0.04f, 0.30f, 0.75f, P::Sky,       P::Sky,       +1, 1 },

    // ── Row 2 (notes 21-28): cool sweeps + pulses / shimmer ───────────────
    { "Cyan Sweep",      MotionType::SWEEP,      0.10f, 0.0f,  0.85f, P::Cyan,      P::Cyan,      +1, 1 },
    { "Green Sweep",     MotionType::SWEEP,      0.08f, 0.0f,  0.80f, P::Green,     P::Green,     -1, 1 },
    { "Indigo Sweep",    MotionType::SWEEP,      0.10f, 0.0f,  0.75f, P::Indigo,    P::Indigo,    +1, 1 },
    { "Mint Sweep",      MotionType::SWEEP,      0.09f, 0.0f,  0.75f, P::Mint,      P::Mint,      -1, 1 },
    { "Teal Pulse",      MotionType::PULSE,      0.10f, 0.0f,  0.80f, P::Teal,      P::Teal,      +1, 1 },
    { "Green Pulse x2",  MotionType::PULSE,      0.10f, 0.0f,  0.75f, P::Green,     P::Green,     +1, 2 },
    { "Violet Pulse x3", MotionType::PULSE,      0.12f, 0.0f,  0.70f, P::Violet,    P::Violet,    -1, 3 },
    { "Cyan Shimmer",    MotionType::ODD_EVEN,   0.20f, 0.0f,  0.75f, P::Cyan,      P::Cyan,      +1, 1 },

    // ── Row 3 (notes 31-38): jewel colour washes ──────────────────────────
    { "Green-Teal Wash", MotionType::COLOUR_WASH,0.06f, 0.0f,  0.90f, P::Green,     P::Teal,      +1, 1 },
    { "Teal-Cyan Wash",  MotionType::COLOUR_WASH,0.05f, 0.0f,  0.85f, P::Teal,      P::Cyan,      +1, 1 },
    { "Blue-Violet Wash",MotionType::COLOUR_WASH,0.07f, 0.0f,  0.85f, P::Blue,      P::Violet,    -1, 1 },
    { "Viol-Mag Wash",   MotionType::COLOUR_WASH,0.06f, 0.0f,  0.85f, P::Violet,    P::Magenta,   +1, 1 },
    { "Cyan-Blue Wash",  MotionType::COLOUR_WASH,0.05f, 0.0f,  0.85f, P::Cyan,      P::Blue,      +1, 1 },
    { "Mint-Sky Wash",   MotionType::COLOUR_WASH,0.04f, 0.0f,  0.80f, P::Mint,      P::Sky,       +1, 1 },
    { "Lime-Green Wash", MotionType::COLOUR_WASH,0.05f, 0.0f,  0.80f, P::Lime,      P::Green,     +1, 1 },
    { "Purple-Pink Wash",MotionType::COLOUR_WASH,0.05f, 0.0f,  0.85f, P::Purple,    P::Pink,      -1, 1 },

    // ── Row 4 (notes 41-48): multi-cycle cool washes + wipes ──────────────
    { "Green-Teal x2",   MotionType::COLOUR_WASH,0.08f, 0.0f,  0.90f, P::Green,     P::Teal,      +1, 2 },
    { "Cyan-Viol x2",    MotionType::COLOUR_WASH,0.07f, 0.0f,  0.85f, P::Cyan,      P::Violet,    -1, 2 },
    { "Teal-Blue x3",    MotionType::COLOUR_WASH,0.09f, 0.0f,  0.85f, P::Teal,      P::Blue,      +1, 3 },
    { "Mag-Cyan Wash",   MotionType::COLOUR_WASH,0.06f, 0.0f,  0.85f, P::Magenta,   P::Cyan,      +1, 1 },
    { "Cyan Wipe",       MotionType::WIPE,       0.12f, 0.0f,  0.90f, P::Cyan,      P::CoolWhite, +1, 1 },
    { "Green Wipe",      MotionType::WIPE,       0.12f, 0.0f,  0.90f, P::Green,     P::Lime,      +1, 1 },
    { "Violet Wipe Dn",  MotionType::WIPE,       0.10f, 0.0f,  0.80f, P::Violet,    P::Lavender,  -1, 1 },
    { "Teal Wipe",       MotionType::WIPE,       0.11f, 0.0f,  0.85f, P::Teal,      P::Sky,       +1, 1 },

    // ── Row 5 (notes 51-58): cool comets & multi-comets ───────────────────
    { "Cyan Comet",      MotionType::COMET,      0.25f, 0.30f, 0.95f, P::Cyan,      P::Cyan,      +1, 1 },
    { "Green Comet Dn",  MotionType::COMET,      0.22f, 0.30f, 0.90f, P::Green,     P::Green,     -1, 1 },
    { "Violet Comet",    MotionType::COMET,      0.28f, 0.28f, 1.00f, P::Violet,    P::Violet,    +1, 1 },
    { "Sky Comet Dn",    MotionType::COMET,      0.25f, 0.28f, 0.85f, P::Sky,       P::Sky,       -1, 1 },
    { "Teal 2-Comet",    MotionType::MULTI_COMET,0.22f, 0.25f, 0.95f, P::Teal,      P::Teal,      +1, 2 },
    { "Purple 2-Comet",  MotionType::MULTI_COMET,0.22f, 0.25f, 0.90f, P::Purple,    P::Purple,    -1, 2 },
    { "Green 3-Comet",   MotionType::MULTI_COMET,0.25f, 0.20f, 0.85f, P::Green,     P::Green,     +1, 3 },
    { "Cyan 3-Comet",    MotionType::MULTI_COMET,0.28f, 0.20f, 1.00f, P::Cyan,      P::Cyan,      +1, 3 },

    // ── Row 6 (notes 61-68): cool quad chases & ping-pongs ────────────────
    { "Cyan Quad Chase", MotionType::QUAD_CHASE, 0.30f, 0.40f, 0.95f, P::Cyan,      P::Cyan,      +1, 1 },
    { "Green Quad Chase",MotionType::QUAD_CHASE, 0.28f, 0.40f, 0.85f, P::Green,     P::Green,     -1, 1 },
    { "Viol Quad Chase", MotionType::QUAD_CHASE, 0.30f, 0.35f, 0.90f, P::Violet,    P::Violet,    +1, 1 },
    { "Teal Quad PP",    MotionType::QUAD_PING_PONG,0.30f,0.45f,0.95f,P::Teal,      P::Teal,      +1, 1 },
    { "Mag Quad PP",     MotionType::QUAD_PING_PONG,0.32f,0.45f,0.90f,P::Magenta,   P::Magenta,   +1, 1 },
    { "Cyan Ping Pong",  MotionType::PING_PONG,  0.25f, 0.30f, 0.90f, P::Cyan,      P::Cyan,      +1, 1 },
    { "Green Ping Pong", MotionType::PING_PONG,  0.30f, 0.28f, 1.00f, P::Green,     P::Green,     +1, 1 },
    { "Indigo Ping Pong",MotionType::PING_PONG,  0.26f, 0.30f, 0.85f, P::Indigo,    P::Indigo,    -1, 1 },

    // ── Row 7 (notes 71-78): wild cool multi-comets, shimmer, fast washes ─
    { "Lime 3-Comet F",  MotionType::MULTI_COMET,0.55f, 0.18f, 1.00f, P::Lime,      P::Lime,      +1, 3 },
    { "Mag 3-Comet F",   MotionType::MULTI_COMET,0.55f, 0.18f, 1.00f, P::Magenta,   P::Magenta,   -1, 3 },
    { "Cyan 4-Comet",    MotionType::MULTI_COMET,0.50f, 0.16f, 1.00f, P::Cyan,      P::Cyan,      +1, 4 },
    { "Violet 4-Comet",  MotionType::MULTI_COMET,0.50f, 0.16f, 1.00f, P::Violet,    P::Violet,    -1, 4 },
    { "Green Shimmer F", MotionType::ODD_EVEN,   0.50f, 0.0f,  1.00f, P::Green,     P::Green,     +1, 1 },
    { "Mag Shimmer F",   MotionType::ODD_EVEN,   0.50f, 0.0f,  1.00f, P::Magenta,   P::Magenta,   +1, 1 },
    { "Cyan Wash F x3",  MotionType::COLOUR_WASH,0.50f, 0.0f,  1.00f, P::Cyan,      P::Blue,      +1, 3 },
    { "Green-Mag F x2",  MotionType::COLOUR_WASH,0.45f, 0.0f,  1.00f, P::Green,     P::Magenta,   +1, 2 },

    // ── Row 8 (notes 81-88): wildest — rainbows & fast ────────────────────
    { "Rainbow Vert Dn", MotionType::RAINBOW,    0.30f, 0.0f,  1.00f, P::Red,       P::Blue,      -1, 1 },
    { "Rainbow Fast Dn", MotionType::RAINBOW,    0.55f, 0.0f,  1.00f, P::Red,       P::Blue,      -1, 1 },
    { "Rainbow x4",      MotionType::RAINBOW,    0.40f, 0.0f,  1.00f, P::Red,       P::Blue,      +1, 4 },
    { "Rainbow x2 Dn",   MotionType::RAINBOW,    0.45f, 0.0f,  1.00f, P::Red,       P::Blue,      -1, 2 },
    { "Cyan Quad PP F",  MotionType::QUAD_PING_PONG,0.60f,0.35f,1.00f,P::Cyan,      P::Cyan,      +1, 1 },
    { "Mag 5-Comet",     MotionType::MULTI_COMET,0.70f, 0.12f, 1.00f, P::Magenta,   P::Magenta,   +1, 5 },
    { "Green Pulse F x4",MotionType::PULSE,      0.60f, 0.0f,  1.00f, P::Green,     P::Green,     +1, 4 },
    { "Violet 5-Comet",  MotionType::MULTI_COMET,0.70f, 0.12f, 1.00f, P::Violet,    P::Violet,    -1, 5 },
}};

// Convert a Launchpad Mini MK3 Programmer-mode note to an IN-PAGE scene index
// (0..63). Returns -1 if the note is not a grid pad (11-88, col digit 1-8).
inline constexpr int noteToSceneIndex(int note) noexcept
{
    const int row = note / 10 - 1;  // 0-based, 0 = bottom
    const int col = note % 10 - 1;  // 0-based
    if (row < 0 || row > 7 || col < 0 || col > 7)
        return -1;
    return row * 8 + col;
}

// The right-hand "Scene Launch" buttons send notes 19, 29, ... 89 in
// Programmer mode (column 9 of each row). We use them as page selectors,
// top-down: row 8 (note 89) = page 0, row 7 (note 79) = page 1, ...
// Returns the 0-based page, or -1 if `note` is not a page button.
inline constexpr int noteToPageIndex(int note) noexcept
{
    const int row = note / 10;       // 1..8
    const int col = note % 10;       // 9 for the scene-launch column
    if (col != 9 || row < 1 || row > 8)
        return -1;
    const int page = 8 - row;        // row 8 -> page 0 (top button)
    return (page >= 0 && page < kNumPages) ? page : -1;
}
inline constexpr int pageButtonNote(int page) noexcept
{
    return (8 - page) * 10 + 9;      // page 0 -> 89, page 1 -> 79, ...
}

}  // namespace hitlaunchdmx
