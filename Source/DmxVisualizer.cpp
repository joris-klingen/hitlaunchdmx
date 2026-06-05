#include "DmxVisualizer.h"

#include "Scene.h"
#include <cmath>

namespace hitlaunchdmx
{

namespace
{
juce::uint8 toByte(float v) noexcept
{
    return static_cast<juce::uint8>(juce::jlimit(0, 255, static_cast<int>(v * 255.0f + 0.5f)));
}
}

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

    // 4 vertical bars side by side, evenly spaced, centred in the component.
    const int labelH  = 14;
    const int marginY = 16;
    const int gap     = 28;                          // space between bars
    const int barH    = h - 2 * marginY - labelH;    // tallest bar that fits
    const int cellH   = juce::jmax(4, barH / kPixelsPerBar);
    const int realBarH = cellH * kPixelsPerBar;
    const int barW    = juce::jlimit(24, 80, (w - (kNumBars + 1) * gap) / kNumBars);

    const int totalW  = kNumBars * barW + (kNumBars - 1) * gap;
    const int originX = (w - totalW) / 2;
    const int originY = (h - realBarH - labelH) / 2;

    for (int barIdx = 0; barIdx < kNumBars; ++barIdx)
    {
        const int x0 = originX + barIdx * (barW + gap);

        const auto& bar = kBars[barIdx];
        for (int pixel = 1; pixel <= bar.pixels; ++pixel)
        {
            const auto ch  = bar.channelsFor(pixel);
            const uint8_t r  = frame.get(ch[0]);
            const uint8_t gv = frame.get(ch[1]);
            const uint8_t b  = frame.get(ch[2]);

            const int row = bar.pixels - pixel;  // pixel 1 at bottom
            g.setColour(juce::Colour(r, gv, b));
            g.fillRect(x0, originY + row * cellH, barW, cellH - 1);
        }

        // Bar label
        g.setColour(juce::Colour(0xff888888));
        g.setFont(juce::FontOptions(10.0f));
        g.drawText("bar " + juce::String(barIdx + 1),
                   x0, originY + realBarH + 2, barW, 12, juce::Justification::centred);
    }
}

}  // namespace hitlaunchdmx
