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

    // The 4 bars sit on a C-arc (0°→60°→120°→180°, open end left).
    // Centre the arc horizontally by shifting the pivot leftward.
    // Average bar x-offset = R * (sin0 + sin60 + sin120 + sin180) / 4 = R*0.433
    // so we shift the draw-centre left by that amount.
    const float cx     = static_cast<float>(w) * 0.5f;
    const float cy     = static_cast<float>(h) * 0.5f;
    const float radius = std::min(cx, cy) * 0.62f;
    const float dcx    = cx - radius * 0.43f;   // compensate for C being right-heavy

    const int barW  = 38;
    const int cellH = juce::jmax(8, static_cast<int>(radius * 0.095f));
    const int barH  = kPixelsPerBar * cellH;

    for (int barIdx = 0; barIdx < kNumBars; ++barIdx)
    {
        const float angle = kBarAngle[barIdx];
        const float bx    = dcx + radius * std::sin(angle);
        const float by    = cy  - radius * std::cos(angle);
        const int   x0    = static_cast<int>(bx) - barW / 2;
        const int   y0    = static_cast<int>(by) - barH / 2;

        const auto& bar = kBars[barIdx];
        for (int pixel = 1; pixel <= bar.pixels; ++pixel)
        {
            const auto ch  = bar.channelsFor(pixel);
            const uint8_t r  = frame.get(ch[0]);
            const uint8_t gv = frame.get(ch[1]);
            const uint8_t b  = frame.get(ch[2]);

            const int row = bar.pixels - pixel;  // pixel 1 at bottom
            g.setColour(juce::Colour(r, gv, b));
            g.fillRect(x0, y0 + row * cellH, barW, cellH - 1);
        }

        // Bar label
        g.setColour(juce::Colour(0xff666666));
        g.setFont(juce::FontOptions(9.0f));
        g.drawText("bar " + juce::String(barIdx + 1),
                   x0, y0 + barH + 2, barW, 12, juce::Justification::centred);
    }

    // Draw faint arc guide so the C shape is legible when bars are dark
    {
        juce::Path arc;
        // arc from 0° to 180° going clockwise (right side), drawn as a polyline
        const int kSegments = 40;
        for (int i = 0; i <= kSegments; ++i)
        {
            const float t = static_cast<float>(i) / kSegments;
            const float a = t * 3.14159265f;  // 0 → π
            const float px = dcx + radius * std::sin(a);
            const float py = cy  - radius * std::cos(a);
            if (i == 0) arc.startNewSubPath(px, py);
            else        arc.lineTo(px, py);
        }
        g.setColour(juce::Colour(0xff2a2a2a));
        g.strokePath(arc, juce::PathStrokeType(1.5f));
    }
}

}  // namespace hitlaunchdmx
