#pragma once

#include "Palette.h"   // hitnotedmx
#include "Recipes.h"   // hitnotedmx

namespace hitlaunchdmx
{

// ── Launchpad Mini MK3 page layout ──────────────────────────────────────────
//
// Pads are addressed in Programmer mode: row 1 (bottom) .. 8 (top), col 1..8,
// note = 10 × row + col. The 8 right-hand Scene Launch buttons (CC 89 top ..
// 19 bottom) pick the page; the 8 top round buttons (CC 91..98) are fixed
// functions (see TopButton).
//
// Every builder pad IS a hitnotedmx note: latching it holds that note in the
// engine. Two builder pages cover the whole vocabulary that makes a look; the
// remaining six pages store whole looks.
//
//   Page 1 Motion   the all-in-one play page:
//                   cols 1-6: rows 8-7 Chases · 6-5 Breathes · 4-3 Wild
//                   (each bank 6 wide × 2 rows: upper row = notes 0-5 of the
//                   octave, lower row = 6-11);
//                   cols 7-8, rows 8-3: 12 primary colours (kMotionColours);
//                   rows 2-1, full width: 16 Multicolor looks (kMotionMulticolor).
//                   No zones or bar selectors on the Launchpad: every bar is
//                   always in play and the XL faders set each bar's level.
//   Page 2 Colour   rows 8-6 Primary 24 · row 5 + row 4 cols 1-4 Secondary 12
//                   · rows 3-1 Multicolor 24 (8 per row).
//   Pages 3-8       Looks: 64 stored looks each (top-left = first).

inline constexpr int kNumPages      = 8;
inline constexpr int kPageMotion    = 0;
inline constexpr int kPageColour    = 1;
inline constexpr int kFirstLookPage = 2;
inline constexpr int kLooksPerPage  = 64;
inline constexpr int kNumLooks      = (kNumPages - kFirstLookPage) * kLooksPerPage;   // 384

inline constexpr const char* kPageNames[kNumPages] {
    "Motion", "Colour", "Looks 1", "Looks 2", "Looks 3", "Looks 4", "Looks 5", "Looks 6"
};

// hitnotedmx structural notes used here (see its mappings/v<N>.tsv).
inline constexpr int kBlackoutPitch  = 0;
inline constexpr int kSpotPitch0     = 1;    // 1 L WW, 2 L col, 3 R WW, 4 R col
inline constexpr int kFromBlackPitch = 9;
inline constexpr int kToBlackPitch   = 10;
inline constexpr int kBumpPitch      = 120;
inline constexpr int kReleasePitch   = 121;
inline constexpr int kCrossfadePitch = 122;
inline constexpr int kFreezePitch    = 123;
inline constexpr int kReversePitch   = 124;
inline constexpr int kFlipPitch      = 125;
inline constexpr int kSpreadPitch    = 126;

// The Motion page's colour column and Multicolor rows — picks from the full
// sets (all of them stay on the Colour page). Edit freely: any primary /
// secondary palette note or Multicolor note works.
//   colours: read left→right, top→bottom over cols 7-8, rows 8-3.
inline constexpr int kMotionColours[12] {
    hitnotedmx::kPrimaryPaletteStart +  1,   // Red
    hitnotedmx::kPrimaryPaletteStart +  3,   // Orange
    hitnotedmx::kPrimaryPaletteStart +  5,   // Yellow
    hitnotedmx::kPrimaryPaletteStart +  7,   // Green
    hitnotedmx::kPrimaryPaletteStart +  9,   // Teal
    hitnotedmx::kPrimaryPaletteStart + 10,   // Cyan
    hitnotedmx::kPrimaryPaletteStart + 12,   // Blue
    hitnotedmx::kPrimaryPaletteStart + 15,   // Violet
    hitnotedmx::kPrimaryPaletteStart + 17,   // Magenta
    hitnotedmx::kPrimaryPaletteStart + 18,   // Pink
    hitnotedmx::kPrimaryPaletteStart + 21,   // Warm white
    hitnotedmx::kPrimaryPaletteStart + 22,   // Cool white
};
//   multicolor: row 2 left→right, then row 1 — the first 16 of the bank.
inline constexpr int kMotionMulticolor[16] {
    hitnotedmx::kColorDynStart +  0, hitnotedmx::kColorDynStart +  1,   // Rainbow, Comet
    hitnotedmx::kColorDynStart +  2, hitnotedmx::kColorDynStart +  3,   // VU meter, VU smooth
    hitnotedmx::kColorDynStart +  4, hitnotedmx::kColorDynStart +  5,   // Fire, Embers
    hitnotedmx::kColorDynStart +  6, hitnotedmx::kColorDynStart +  7,   // Magma, Lava
    hitnotedmx::kColorDynStart +  8, hitnotedmx::kColorDynStart +  9,   // Heatmap, Ocean
    hitnotedmx::kColorDynStart + 10, hitnotedmx::kColorDynStart + 11,   // Forest, Desert
    hitnotedmx::kColorDynStart + 12, hitnotedmx::kColorDynStart + 13,   // Sunset, Twilight
    hitnotedmx::kColorDynStart + 14, hitnotedmx::kColorDynStart + 15,   // Borealis, Night sky
};

// Pitch → pad on a builder page, or -1.
inline int padPitch (int page, int row, int col) noexcept
{
    if (row < 1 || row > 8 || col < 1 || col > 8)
        return -1;

    if (page == kPageMotion)
    {
        if (row <= 2)                                // Multicolor, full width
            return kMotionMulticolor[(2 - row) * 8 + (col - 1)];
        if (col >= 7)                                // colour column
            return kMotionColours[(8 - row) * 2 + (col - 7)];

        const int bankRow = (8 - row) / 2;           // 0 Chases, 1 Breathes, 2 Wild
        const bool upper  = ((8 - row) % 2) == 0;
        static constexpr int bankStart[3] = { hitnotedmx::kChasesStart, hitnotedmx::kBreathesStart,
                                              hitnotedmx::kWildStart };
        return bankStart[bankRow] + (upper ? 0 : 6) + (col - 1);
    }

    if (page == kPageColour)
    {
        if (row >= 6)                                // primary 24
            return hitnotedmx::kPrimaryPaletteStart + (8 - row) * 8 + (col - 1);
        if (row == 5)
            return hitnotedmx::kSecondaryPaletteStart + (col - 1);
        if (row == 4)
            return col <= 4 ? hitnotedmx::kSecondaryPaletteStart + 8 + (col - 1) : -1;
        return hitnotedmx::kColorDynStart + (3 - row) * 8 + (col - 1);   // rows 3..1
    }

    return -1;
}

// Look slot for a pad on a look page, or -1.
inline int padLook (int page, int row, int col) noexcept
{
    if (page < kFirstLookPage || page >= kNumPages || row < 1 || row > 8 || col < 1 || col > 8)
        return -1;
    return (page - kFirstLookPage) * kLooksPerPage + (8 - row) * 8 + (col - 1);
}

// Exclusivity group of a latchable pitch: tapping a pad (without Shift) clears
// the other pads in its group. -1 = no group (spots toggle independently).
inline int pitchGroup (int p) noexcept
{
    if (hitnotedmx::isChasesPitch (p))                              return 3;
    if (hitnotedmx::isBreathesPitch (p))                            return 4;
    if (hitnotedmx::isWildPitch (p))                                return 5;
    if (hitnotedmx::isColorDynPitch (p))                            return 6;
    if (p >= hitnotedmx::kPrimaryPaletteStart && p < hitnotedmx::kSecondaryPaletteStart) return 7;
    if (p >= hitnotedmx::kSecondaryPaletteStart && p < hitnotedmx::kSecondaryPaletteEnd) return 8;
    return -1;
}


// Top round buttons (CC 91..98).
enum class TopButton : int { SpotLWW = 0, SpotLCol, SpotRWW, SpotRCol, Clear, Store, Shift, Blackout };
inline constexpr const char* kTopButtonNames[8] {
    "Spot L WW", "Spot L col", "Spot R WW", "Spot R col", "Clear", "Store", "Shift", "Blackout"
};

}  // namespace hitlaunchdmx
