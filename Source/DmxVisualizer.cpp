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

    // ONE long vertical line of 72 LEDs: 4 bars stacked, bar 1 at the bottom,
    // bar 4 at the top, with a small gap between bar groups so the 4-bar
    // structure is still visible.
    const int marginY  = 16;
    const int barGap   = 8;                                   // gap between bar groups
    const int totalGap = (kNumBars - 1) * barGap;
    const int avail    = h - 2 * marginY - totalGap;
    const int cellH    = juce::jmax(3, avail / kNumLeds);     // per-LED height
    const int groupH   = cellH * kPixelsPerBar;               // 18 LEDs tall
    const int stackH   = kNumBars * groupH + totalGap;
    const int barW     = juce::jlimit(30, 120, w / 3);
    const int x0       = (w - barW) / 2;
    const int topY     = (h - stackH) / 2;                    // top of the whole stack

    for (int barIdx = 0; barIdx < kNumBars; ++barIdx)
    {
        // barIdx 0 sits at the BOTTOM of the stack, barIdx 3 at the top.
        const int groupTop = topY + (kNumBars - 1 - barIdx) * (groupH + barGap);

        const auto& bar = kBars[barIdx];
        for (int pixel = 1; pixel <= bar.pixels; ++pixel)
        {
            const auto ch  = bar.channelsFor(pixel);
            const uint8_t r  = frame.get(ch[0]);
            const uint8_t gv = frame.get(ch[1]);
            const uint8_t b  = frame.get(ch[2]);

            const int row = bar.pixels - pixel;  // pixel 1 at the bottom of its group
            g.setColour(juce::Colour(r, gv, b));
            g.fillRect(x0, groupTop + row * cellH, barW, cellH - 1);
        }

        // Bar number to the left of each group
        g.setColour(juce::Colour(0xff777777));
        g.setFont(juce::FontOptions(10.0f));
        g.drawText(juce::String(barIdx + 1),
                   x0 - 22, groupTop + groupH / 2 - 7, 16, 14,
                   juce::Justification::centredRight);
    }
}

}  // namespace hitlaunchdmx
