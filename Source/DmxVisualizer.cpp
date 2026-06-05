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

    // Ring parameters
    const float cx      = static_cast<float>(w) * 0.5f;
    const float cy      = static_cast<float>(h) * 0.5f;
    const float radius  = std::min(cx, cy) * 0.60f;
    const int   barW    = 36;
    const int   cellH   = static_cast<int>(radius * 0.09f);
    const int   barH    = kPixelsPerBar * cellH;

    // Draw each bar centred at its clock position
    for (int barIdx = 0; barIdx < kNumBars; ++barIdx)
    {
        const float angle = kBarAngle[barIdx];  // radians, 0 = top, CW
        // Centre of bar strip
        const float bx = cx + radius * std::sin(angle);
        const float by = cy - radius * std::cos(angle);
        const int   x0 = static_cast<int>(bx) - barW / 2;
        const int   y0 = static_cast<int>(by) - barH / 2;

        const auto& bar = kBars[barIdx];
        for (int pixel = 1; pixel <= bar.pixels; ++pixel)
        {
            const auto ch = bar.channelsFor(pixel);
            const uint8_t r = frame.get(ch[0]);
            const uint8_t gv = frame.get(ch[1]);
            const uint8_t b  = frame.get(ch[2]);

            // pixel 1 is the bottom of the bar
            const int row = bar.pixels - pixel;
            g.setColour(juce::Colour(r, gv, b));
            g.fillRect(x0, y0 + row * cellH, barW, cellH - 1);
        }

        // Bar label
        g.setColour(juce::Colours::grey);
        g.setFont(juce::FontOptions(9.0f));
        const juce::String label = "bar " + juce::String(barIdx + 1);
        g.drawText(label, x0, y0 + barH + 2, barW, 12, juce::Justification::centred);
    }

    // Scene name in the centre
    if (activeSceneIdx >= 0 && activeSceneIdx < 64)
    {
        const auto& sc = kScenes[static_cast<size_t>(activeSceneIdx)];
        g.setColour(juce::Colour(0xffaaaaaa));
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(juce::String(sc.name),
                   static_cast<int>(cx) - 70, static_cast<int>(cy) - 8, 140, 16,
                   juce::Justification::centred);
    }
}

}  // namespace hitlaunchdmx
