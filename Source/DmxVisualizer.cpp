#include "DmxVisualizer.h"

#include "Scene.h"
#include <cmath>

namespace hitlaunchdmx
{

DmxVisualizer::DmxVisualizer(const DmxFrame& frameRef, int /*activeSceneRef*/)
    : frame(frameRef)
{
    setOpaque(true);
    lastFingerprint.fill(0);
}

void DmxVisualizer::resized()
{
    const int w = getWidth(), h = getHeight();
    if (w > 0 && h > 0)
        cachedImage = juce::Image(juce::Image::RGB, w, h, false);
    else
        cachedImage = juce::Image();
    rebuildCache();
}

void DmxVisualizer::paint(juce::Graphics& g)
{
    if (cachedImage.isValid())
        g.drawImageAt(cachedImage, 0, 0);
    else
        g.fillAll(juce::Colour(0xff141414));
}

void DmxVisualizer::repaintIfChanged()
{
    std::array<uint8_t, kFingerprintSize> current {};
    int idx = 0;
    for (int b = 0; b < kNumBars; ++b)
    {
        const auto& bar = kBars[b];
        for (int p = 1; p <= bar.pixels; ++p)
        {
            const auto ch = bar.channelsFor(p);
            current[idx++] = frame.get(ch[0]);
            current[idx++] = frame.get(ch[1]);
            current[idx++] = frame.get(ch[2]);
        }
    }
    if (current == lastFingerprint) return;
    lastFingerprint = current;
    rebuildCache();
    repaint();
}

void DmxVisualizer::rebuildCache()
{
    if (!cachedImage.isValid()) return;

    const int w = cachedImage.getWidth();
    const int h = cachedImage.getHeight();
    juce::Graphics g(cachedImage);
    g.fillAll(juce::Colour(0xff141414));

    // Grid style (matches hitnotedmx): two RGBW spots flank a grid of vertical
    // bars. The bars are drawn as kNumBars columns side by side, each a stack
    // of kPixelsPerBar cells with pixel 1 at the BOTTOM (bottom-up rig). The
    // spots are reserved-but-dark in this app, so they read as faint outlines.
    const int marginX = 14;
    const int marginY = 16;
    const int spotDia = juce::jlimit(14, 40, w / 14);   // spot circle diameter
    const int spotGap = 12;                              // gap spot ↔ grid

    // Horizontal band available to the bar grid (spots sit outside it).
    const int gridLeft  = marginX + spotDia + spotGap;
    const int gridRight = w - marginX - spotDia - spotGap;
    const int gridW     = juce::jmax(kNumBars * 6, gridRight - gridLeft);

    const int barGap = juce::jmax(4, gridW / (kNumBars * 6));   // gap between columns
    const int barW   = juce::jmax(6, (gridW - (kNumBars - 1) * barGap) / kNumBars);
    const int gridUsedW = kNumBars * barW + (kNumBars - 1) * barGap;
    const int gridX0    = gridLeft + (gridW - gridUsedW) / 2;

    const int avail = h - 2 * marginY;
    const int cellH = juce::jmax(3, avail / kPixelsPerBar);
    const int colH  = cellH * kPixelsPerBar;
    const int topY  = (h - colH) / 2;

    // --- bar grid ---------------------------------------------------------
    for (int barIdx = 0; barIdx < kNumBars; ++barIdx)
    {
        const int colX  = gridX0 + barIdx * (barW + barGap);
        const auto& bar = kBars[barIdx];

        for (int pixel = 1; pixel <= bar.pixels; ++pixel)
        {
            const auto ch  = bar.channelsFor(pixel);
            const uint8_t r  = frame.get(ch[0]);
            const uint8_t gv = frame.get(ch[1]);
            const uint8_t b  = frame.get(ch[2]);

            const int row = bar.pixels - pixel;  // pixel 1 at the bottom
            g.setColour(juce::Colour(r, gv, b));
            g.fillRect(colX, topY + row * cellH, barW, cellH - 1);
        }

        // Bar number under each column.
        g.setColour(juce::Colour(0xff777777));
        g.setFont(juce::FontOptions(10.0f));
        g.drawText(juce::String(barIdx + 1),
                   colX, topY + colH + 2, barW, 12,
                   juce::Justification::centredTop);
    }

    // --- two RGBW spots flanking the grid ---------------------------------
    const int spotY = (h - spotDia) / 2;
    const auto drawSpot = [&](int spotIdx, int cx)
    {
        const auto& sp = kSpots[spotIdx];
        // Composite the RGBW spot to an on-screen colour (W adds warm white).
        const int r = frame.get(sp.red());
        const int gv= frame.get(sp.green());
        const int b = frame.get(sp.blue());
        const int wv= frame.get(sp.white());
        const juce::uint8 rr = (juce::uint8) juce::jmin(255, r + (wv * 255) / 255);
        const juce::uint8 gg = (juce::uint8) juce::jmin(255, gv + (wv * 244) / 255);
        const juce::uint8 bb = (juce::uint8) juce::jmin(255, b + (wv * 224) / 255);

        const auto bounds = juce::Rectangle<float>((float) cx, (float) spotY,
                                                   (float) spotDia, (float) spotDia);
        g.setColour(juce::Colour(rr, gg, bb));
        g.fillEllipse(bounds);
        g.setColour(juce::Colour(0xff444444));   // faint ring marks a reserved spot
        g.drawEllipse(bounds, 1.5f);
    };
    drawSpot(0, marginX);                       // spot_l, far left
    drawSpot(1, w - marginX - spotDia);         // spot_r, far right
}

}  // namespace hitlaunchdmx
