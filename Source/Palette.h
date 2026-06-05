#pragma once

#include <array>

namespace hitlaunchdmx
{

struct PaletteColor { float r, g, b; };

// Named colours used when composing scene presets.
namespace Pal
{
    inline constexpr PaletteColor Black       { 0.000f, 0.000f, 0.000f };
    inline constexpr PaletteColor Red         { 1.000f, 0.000f, 0.000f };
    inline constexpr PaletteColor OrangeRed   { 1.000f, 0.235f, 0.000f };
    inline constexpr PaletteColor Orange      { 1.000f, 0.471f, 0.000f };
    inline constexpr PaletteColor Amber       { 1.000f, 0.706f, 0.000f };
    inline constexpr PaletteColor Yellow      { 1.000f, 0.902f, 0.000f };
    inline constexpr PaletteColor Lime        { 0.706f, 1.000f, 0.000f };
    inline constexpr PaletteColor Green       { 0.000f, 1.000f, 0.000f };
    inline constexpr PaletteColor Mint        { 0.000f, 1.000f, 0.471f };
    inline constexpr PaletteColor Teal        { 0.000f, 0.784f, 0.706f };
    inline constexpr PaletteColor Cyan        { 0.000f, 0.863f, 1.000f };
    inline constexpr PaletteColor Sky         { 0.000f, 0.549f, 1.000f };
    inline constexpr PaletteColor Blue        { 0.000f, 0.000f, 1.000f };
    inline constexpr PaletteColor Royal       { 0.235f, 0.000f, 0.902f };
    inline constexpr PaletteColor Indigo      { 0.392f, 0.000f, 0.784f };
    inline constexpr PaletteColor Violet      { 0.627f, 0.000f, 0.863f };
    inline constexpr PaletteColor Purple      { 0.745f, 0.000f, 0.745f };
    inline constexpr PaletteColor Magenta     { 1.000f, 0.000f, 0.784f };
    inline constexpr PaletteColor Pink        { 1.000f, 0.392f, 0.706f };
    inline constexpr PaletteColor HotPink     { 1.000f, 0.157f, 0.471f };
    inline constexpr PaletteColor Crimson     { 0.706f, 0.000f, 0.157f };
    inline constexpr PaletteColor WarmWhite   { 1.000f, 0.706f, 0.431f };
    inline constexpr PaletteColor CoolWhite   { 0.863f, 0.902f, 1.000f };
    inline constexpr PaletteColor Lavender    { 0.784f, 0.706f, 1.000f };
    inline constexpr PaletteColor SoftRose    { 1.000f, 0.600f, 0.600f };
    inline constexpr PaletteColor SoftAmber   { 0.800f, 0.500f, 0.100f };
    inline constexpr PaletteColor DimWhite    { 0.500f, 0.440f, 0.270f };
    inline constexpr PaletteColor SoftBlue    { 0.200f, 0.400f, 0.800f };
}

// Launchpad Mini MK3 Programmer-mode colour bytes (velocity values).
// Approximate mapping from Novation SysEx colour palette.
namespace LpColor
{
    inline constexpr uint8_t Off        =   0;
    inline constexpr uint8_t DimAmber   =   9;
    inline constexpr uint8_t Amber      =  10;
    inline constexpr uint8_t Yellow     =  13;
    inline constexpr uint8_t DimGreen   =  17;
    inline constexpr uint8_t Green      =  21;
    inline constexpr uint8_t Teal       =  37;
    inline constexpr uint8_t Cyan       =  90;
    inline constexpr uint8_t Sky        =  41;
    inline constexpr uint8_t DimBlue    =  43;
    inline constexpr uint8_t Blue       =  45;
    inline constexpr uint8_t Violet     =  49;
    inline constexpr uint8_t Magenta    =  53;
    inline constexpr uint8_t Pink       =  56;
    inline constexpr uint8_t White      =   3;
    inline constexpr uint8_t WarmWhite  =   4;
    inline constexpr uint8_t ActiveBrt  = 127;
    // Category idle colours
    inline constexpr uint8_t BreatheCat =  DimAmber;
    inline constexpr uint8_t MedCat     =  DimBlue;
    inline constexpr uint8_t WildCat    =  DimGreen;
}

}  // namespace hitlaunchdmx
