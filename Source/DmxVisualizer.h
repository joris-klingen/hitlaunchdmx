#pragma once

#include <array>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Rig.h"
#include "SceneEngine.h"  // DmxFrame

namespace hitlaunchdmx
{

// On-screen preview (grid style, matching hitnotedmx): two RGBW spots flank a
// grid of kNumBars vertical bars, each drawn as a column of kPixelsPerBar
// coloured cells with pixel 1 at the bottom. The spots are reserved-but-dark
// in this app (see Rig.h) so they read as faint outlines. Pre-rendered into a
// cached juce::Image; paint() is one blit.
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
