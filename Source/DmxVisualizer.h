#pragma once

#include <array>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Rig.h"
#include "SceneEngine.h"  // DmxFrame

namespace hitlaunchdmx
{

// On-screen preview: 4 vertical bars arranged at 12/3/6/9 o'clock around
// a circle. Each bar is drawn as a column of 18 coloured cells, pixel 1 at
// the bottom. Pre-rendered into a cached juce::Image; paint() is one blit.
class DmxVisualizer : public juce::Component
{
public:
    explicit DmxVisualizer(const DmxFrame& frameRef, int activeSceneRef);

    void paint(juce::Graphics&) override;
    void resized() override;

    // Call from the GUI timer (15 Hz). Rebuilds the cache if the frame changed.
    void repaintIfChanged();

    // Set the currently-active scene so its name can be shown.
    void setActiveScene(int idx) { activeSceneIdx = idx; }

private:
    void rebuildCache();

    static constexpr int kFingerprintSize = kNumBars * kPixelsPerBar * 3;

    const DmxFrame& frame;
    int             activeSceneIdx { 0 };

    juce::Image cachedImage;
    std::array<uint8_t, kFingerprintSize> lastFingerprint {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DmxVisualizer)
};

}  // namespace hitlaunchdmx
