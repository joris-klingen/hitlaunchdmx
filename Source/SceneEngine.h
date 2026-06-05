#pragma once

#include <array>
#include <atomic>
#include <functional>
#include <juce_audio_utils/juce_audio_utils.h>

#include "EnttecProDmx.h"
#include "MidiLog.h"
#include "Rig.h"
#include "Scene.h"

namespace hitlaunchdmx
{

// Per-channel live DMX frame — written by SceneEngine at 60 Hz,
// read by DmxVisualizer on the GUI thread (read-only, best-effort).
struct DmxFrame
{
    static constexpr int kSize = kDmxUniverseSize;

    void set(int ch1based, float v) noexcept
    {
        if (ch1based >= 1 && ch1based <= kSize)
            data[ch1based - 1].store(static_cast<uint8_t>(
                juce::jlimit(0, 255, static_cast<int>(v * 255.0f + 0.5f))),
                std::memory_order_relaxed);
    }

    uint8_t get(int ch1based) const noexcept
    {
        if (ch1based >= 1 && ch1based <= kSize)
            return data[ch1based - 1].load(std::memory_order_relaxed);
        return 0;
    }

private:
    std::array<std::atomic<uint8_t>, kSize> data {};
};

// Drives scene rendering at 60 Hz:
//   • Reads activeScene (set atomically from the MIDI callback).
//   • Calls the parametric generator for every bar/pixel → targetFrame[].
//   • LERPs liveFrame toward targetFrame at rate 1/crossfadeSec per second.
//   • Pushes liveFrame into EnttecProDmx::setChannel.
//   • Also opens/manages the MIDI input device and MIDI output (Launchpad LED
//     feedback).
class SceneEngine : public juce::MidiInputCallback,
                    private juce::HighResolutionTimer
{
public:
    // crossfadeSec: exponential lerp time constant (≈ time to 63% of target)
    explicit SceneEngine(EnttecProDmx& dmx,
                         DmxFrame&     liveFrameOut,
                         MidiLog&      midiLog,
                         float         crossfadeSec = 2.0f);
    ~SceneEngine() override;

    // Start/stop the 60 Hz render loop.
    void start();
    void stop();

    // MIDI device management — call from the GUI thread.
    juce::StringArray getMidiInputNames() const;
    juce::StringArray getMidiOutputNames() const;
    void openMidiInput(const juce::String& name);
    void openMidiOutput(const juce::String& name);
    void closeMidiInput();
    void closeMidiOutput();
    juce::String activeMidiInputName()  const { return openInputName; }
    juce::String activeMidiOutputName() const { return openOutputName; }

    int  getActiveScene()           const noexcept { return activeScene.load(); }
    void setActiveScene(int idx)          noexcept;   // callable from GUI thread
    void setCrossfadeSec(float s)         noexcept  { crossfadeSec.store(s); }

    // Live performance controls (all 1.0 = neutral / default).
    void setGlobalDim(float d)            noexcept  { globalDim.store(d); }   // 0..1 master brightness
    void setSpeedScale(float s)           noexcept  { speedScale.store(s); }  // global motion-rate multiplier
    void setTailScale(float t)            noexcept  { tailScale.store(t); }   // chase/comet tail length multiplier

    // Launchpad setup: send Programmer-mode SysEx so pads send fixed notes.
    void sendLaunchpadProgrammerMode();

    // Refresh all Launchpad LED colours (idle per category + bright for active).
    void refreshLaunchpadLEDs();

private:
    // juce::MidiInputCallback
    void handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage&) override;

    // juce::HighResolutionTimer
    void hiResTimerCallback() override;

    // Compute RGB for one pixel of one bar from the given scene at time t.
    // speedScale multiplies all motion rates; tailScale stretches comet tails.
    static void renderPixel(const Scene& scene, double t,
                            int barIdx, int pixel,
                            float speedScale, float tailScale,
                            float& outR, float& outG, float& outB) noexcept;

    EnttecProDmx& dmx;
    DmxFrame&     liveFrameOut;
    MidiLog&      midiLog;

    std::atomic<int>   activeScene  { 0 };
    std::atomic<float> crossfadeSec { 2.0f };
    std::atomic<float> globalDim    { 1.0f };
    std::atomic<float> speedScale   { 1.0f };
    std::atomic<float> tailScale    { 1.0f };

    // Live and target frames — float [0..1], 1-based channel index.
    std::array<float, kRigChannels> liveFrame  {};
    std::array<float, kRigChannels> targetFrame {};

    std::unique_ptr<juce::MidiInput>  midiIn;
    std::unique_ptr<juce::MidiOutput> midiOut;
    juce::String openInputName;
    juce::String openOutputName;

    double lastTickSec { 0.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SceneEngine)
};

}  // namespace hitlaunchdmx
