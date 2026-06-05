#include "SceneEngine.h"

#include <cmath>
#include <algorithm>

namespace hitlaunchdmx
{

namespace
{
constexpr float kTwoPi = 6.28318530717958647692f;
constexpr int   kRenderHz = 60;

// Gaussian falloff centred at `centre` on a ring of `len` elements.
// `halfWidth` is in ring-fraction units [0..1]; returns [0..1].
float gaussRing(float pos, float centre, float len, float halfWidth) noexcept
{
    float d = std::fmod(std::abs(pos - centre), len);
    if (d > len * 0.5f) d = len - d;
    const float sigma = halfWidth * len;
    return sigma > 0.0f ? std::exp(-(d * d) / (2.0f * sigma * sigma)) : (d < 0.5f ? 1.0f : 0.0f);
}

// Comet brightness: `behind` cells behind the head on a track of `trackLen`.
float cometBrightness(int behind, int trackLen, float width) noexcept
{
    if (behind < 0) return 0.0f;
    const float tailLen = width * static_cast<float>(trackLen);
    if (tailLen <= 0.0f) return behind == 0 ? 1.0f : 0.0f;
    const float b = 1.0f - static_cast<float>(behind) / tailLen;
    return b > 0.0f ? b : 0.0f;
}

int behindWrapped(int pos, int pixel, int len, int dir) noexcept
{
    const int raw = dir > 0 ? (pos - pixel) : (pixel - pos);
    return ((raw % len) + len) % len;
}

// Simple hue→RGB (H in [0..1]).
PaletteColor hsvToRgb(float h, float s, float v) noexcept
{
    h = std::fmod(h, 1.0f) * 6.0f;
    const int   i = static_cast<int>(h);
    const float f = h - static_cast<float>(i);
    const float p = v * (1.0f - s);
    const float q = v * (1.0f - s * f);
    const float t = v * (1.0f - s * (1.0f - f));
    switch (i % 6)
    {
        case 0: return { v, t, p };
        case 1: return { q, v, p };
        case 2: return { p, v, t };
        case 3: return { p, q, v };
        case 4: return { t, p, v };
        default:return { v, p, q };
    }
}
} // namespace

// ---------------------------------------------------------------------------
// Per-pixel generator — pure function of (scene, t, barIdx, pixel).
// barIdx 0-based; pixel 1-based (1 = bottom).
// ---------------------------------------------------------------------------
void SceneEngine::renderPixel(const Scene& sc, double t,
                              int barIdx, int pixel,
                              float speedScale, float tailScale,
                              float& outR, float& outG, float& outB) noexcept
{
    const int   nPix      = kPixelsPerBar;
    const int   nBars     = kNumBars;
    const float baseSpeed = sc.speed * speedScale;                    // scaled motion rate
    const float speed     = baseSpeed * static_cast<float>(sc.direction);
    const float ft        = static_cast<float>(t) * std::abs(baseSpeed);
    const float fts       = static_cast<float>(t) * speed;            // signed speed
    const float cometW    = juce::jlimit(0.02f, 1.0f, sc.width * tailScale);  // scaled tail length

    float brightness = 0.0f;
    PaletteColor col = sc.color1;

    switch (sc.motion)
    {
    // ── BREATHE ──────────────────────────────────────────────────────────
    case MotionType::BREATHE:
        brightness = (1.0f + std::sin(kTwoPi * ft)) * 0.5f;
        col = sc.color1;
        break;

    // ── MOON_RISE ─────────────────────────────────────────────────────────
    // A glow that climbs vertically: sinusoidal vertical envelope that moves up.
    case MotionType::MOON_RISE:
    {
        const float phase   = std::fmod(ft, 1.0f);
        const float centre  = phase * static_cast<float>(nPix + 4) - 2.0f;  // −2..nPix+2
        const float pf      = static_cast<float>(pixel);
        const float sigma   = static_cast<float>(nPix) * 0.25f;
        const float d       = pf - centre;
        brightness = std::exp(-(d * d) / (2.0f * sigma * sigma));
        col = sc.color1;
        break;
    }

    // ── DRIFT_BALL ────────────────────────────────────────────────────────
    // Gaussian blob travels around the ring; vertical envelope is a fixed broad Gaussian.
    case MotionType::DRIFT_BALL:
    {
        const float ringPos  = std::fmod(fts * static_cast<float>(nBars), static_cast<float>(nBars));
        const float ringDist = gaussRing(static_cast<float>(barIdx), ringPos, static_cast<float>(nBars), sc.width);
        const float vertCentre = static_cast<float>(nPix) * 0.5f + 0.5f;
        const float d          = static_cast<float>(pixel) - vertCentre;
        const float sigma      = static_cast<float>(nPix) * 0.30f;
        const float vertFall   = std::exp(-(d * d) / (2.0f * sigma * sigma));
        brightness = ringDist * vertFall;
        col = sc.color1;
        break;
    }

    // ── RING_ROTATE ───────────────────────────────────────────────────────
    // Full-bar lit highlight sweeps around the ring.
    case MotionType::RING_ROTATE:
    {
        const float ringPos = std::fmod(fts * static_cast<float>(nBars), static_cast<float>(nBars));
        brightness = gaussRing(static_cast<float>(barIdx), ringPos, static_cast<float>(nBars), 0.30f);
        col = sc.color1;
        break;
    }

    // ── RING_COMET ────────────────────────────────────────────────────────
    // Comet with tail travels bar-by-bar around the ring; all pixels of a bar lit together.
    case MotionType::RING_COMET:
    {
        const int   headBar = static_cast<int>(fts * static_cast<float>(nBars)) % nBars;
        const int   adjHead = ((headBar % nBars) + nBars) % nBars;
        const int   behind  = behindWrapped(adjHead, barIdx, nBars, sc.direction);
        brightness = cometBrightness(behind, nBars, cometW) *
                     (0.5f + 0.5f * static_cast<float>(pixel) / static_cast<float>(nPix));
        col = sc.color1;
        break;
    }

    // ── VERT_CHASE ────────────────────────────────────────────────────────
    // Vertical comet, all bars in phase.
    case MotionType::VERT_CHASE:
    {
        const int headPix = ((static_cast<int>(fts * static_cast<float>(nPix)) % nPix) + nPix) % nPix + 1;
        const int behind  = behindWrapped(headPix, pixel, nPix, sc.direction);
        brightness = cometBrightness(behind, nPix, cometW);
        col = sc.color1;
        break;
    }

    // ── VERT_CHASE_STAGGER ────────────────────────────────────────────────
    // Vertical comet with each bar phase-offset by 90° around the ring.
    case MotionType::VERT_CHASE_STAGGER:
    {
        const float offset   = static_cast<float>(barIdx) / static_cast<float>(nBars);
        const float tOff     = fts + offset / juce::jmax(0.0001f, std::abs(baseSpeed));
        const int   headPix  = ((static_cast<int>(tOff * static_cast<float>(nPix)) % nPix) + nPix) % nPix + 1;
        const int   behind   = behindWrapped(headPix, pixel, nPix, sc.direction);
        brightness = cometBrightness(behind, nPix, cometW);
        col = sc.color1;
        break;
    }

    // ── COLOUR_WASH ───────────────────────────────────────────────────────
    // Sinusoidal cross-fade between color1 and color2, staggered across bars.
    case MotionType::COLOUR_WASH:
    {
        const float barOff = static_cast<float>(barIdx) / static_cast<float>(nBars);
        const float mix    = (1.0f + std::sin(kTwoPi * (ft + barOff))) * 0.5f;
        col = { sc.color1.r + mix * (sc.color2.r - sc.color1.r),
                sc.color1.g + mix * (sc.color2.g - sc.color1.g),
                sc.color1.b + mix * (sc.color2.b - sc.color1.b) };
        brightness = 1.0f;
        break;
    }

    // ── PULSE_RING ────────────────────────────────────────────────────────
    // Sinusoidal pulse in the ring direction; vertical full on.
    case MotionType::PULSE_RING:
    {
        const float barOff = static_cast<float>(barIdx) / static_cast<float>(nBars);
        brightness = (1.0f + std::sin(kTwoPi * (ft * static_cast<float>(sc.direction) + barOff))) * 0.5f;
        col = sc.color1;
        break;
    }

    // ── RAINBOW_RING ──────────────────────────────────────────────────────
    // Hue progresses around the ring over time; full brightness.
    case MotionType::RAINBOW_RING:
    {
        const float hueBase   = std::fmod(ft, 1.0f);
        const float hueOffset = static_cast<float>(barIdx) / static_cast<float>(nBars);
        const float vertOff   = static_cast<float>(pixel - 1) / static_cast<float>(nPix) * 0.1f;
        const float hue = std::fmod(hueBase * static_cast<float>(sc.direction) + hueOffset + vertOff + 1.0f, 1.0f);
        col = hsvToRgb(hue, 1.0f, 1.0f);
        brightness = 1.0f;
        break;
    }

    // ── DIAGONAL_WIPE ─────────────────────────────────────────────────────
    // Vertical fill staggered per bar; crossfades between color1 and color2.
    case MotionType::DIAGONAL_WIPE:
    {
        const float barOff   = static_cast<float>(barIdx) / static_cast<float>(nBars) * 0.5f;
        const float phase    = std::fmod(ft + barOff, 1.0f);
        const int   fill     = static_cast<int>(phase * static_cast<float>(nPix));
        const float mix      = (fill > 0) ? static_cast<float>(fill) / static_cast<float>(nPix) : 0.0f;
        col = { sc.color1.r + mix * (sc.color2.r - sc.color1.r),
                sc.color1.g + mix * (sc.color2.g - sc.color1.g),
                sc.color1.b + mix * (sc.color2.b - sc.color1.b) };
        brightness = sc.direction > 0 ? (pixel <= fill ? 1.0f : 0.0f)
                                       : (pixel >= (nPix - fill) ? 1.0f : 0.0f);
        break;
    }
    }

    // Apply master brightness
    brightness *= sc.brightness;
    outR = juce::jlimit(0.0f, 1.0f, col.r * brightness);
    outG = juce::jlimit(0.0f, 1.0f, col.g * brightness);
    outB = juce::jlimit(0.0f, 1.0f, col.b * brightness);
}

// ---------------------------------------------------------------------------
SceneEngine::SceneEngine(EnttecProDmx& dmx_,
                         DmxFrame&     liveFrameOut_,
                         MidiLog&      midiLog_,
                         float         crossfadeSec_)
    : dmx(dmx_), liveFrameOut(liveFrameOut_), midiLog(midiLog_)
{
    crossfadeSec.store(crossfadeSec_);
    liveFrame.fill(0.0f);
    targetFrame.fill(0.0f);
}

SceneEngine::~SceneEngine()
{
    stop();
    closeMidiInput();
    closeMidiOutput();
}

void SceneEngine::start()
{
    lastTickSec = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    startTimer(1000 / kRenderHz);
}

void SceneEngine::stop()
{
    stopTimer();
}

// ---------------------------------------------------------------------------
// MIDI input
// ---------------------------------------------------------------------------
juce::StringArray SceneEngine::getMidiInputNames() const
{
    juce::StringArray names;
    for (const auto& d : juce::MidiInput::getAvailableDevices())
        names.add(d.name);
    return names;
}

juce::StringArray SceneEngine::getMidiOutputNames() const
{
    juce::StringArray names;
    for (const auto& d : juce::MidiOutput::getAvailableDevices())
        names.add(d.name);
    return names;
}

void SceneEngine::openMidiInput(const juce::String& name)
{
    closeMidiInput();
    for (const auto& d : juce::MidiInput::getAvailableDevices())
    {
        if (d.name == name)
        {
            midiIn = juce::MidiInput::openDevice(d.identifier, this);
            if (midiIn) { midiIn->start(); openInputName = name; }
            break;
        }
    }
}

void SceneEngine::openMidiOutput(const juce::String& name)
{
    closeMidiOutput();
    for (const auto& d : juce::MidiOutput::getAvailableDevices())
    {
        if (d.name == name)
        {
            midiOut = juce::MidiOutput::openDevice(d.identifier);
            if (midiOut) { openOutputName = name; }
            break;
        }
    }
}

void SceneEngine::closeMidiInput()
{
    if (midiIn) { midiIn->stop(); midiIn.reset(); }
    openInputName = {};
}

void SceneEngine::closeMidiOutput()
{
    midiOut.reset();
    openOutputName = {};
}

void SceneEngine::handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& msg)
{
    MidiLogEntry entry;
    if (msg.isNoteOn())
    {
        entry.kind     = MidiLogEntry::Kind::NoteOn;
        entry.channel  = static_cast<juce::uint8>(msg.getChannel());
        entry.pitch    = static_cast<juce::uint8>(msg.getNoteNumber());
        entry.velocity = static_cast<juce::uint8>(msg.getVelocity());
        midiLog.push(entry);

        const int idx = noteToSceneIndex(msg.getNoteNumber());
        if (idx >= 0 && idx < 64)
        {
            activeScene.store(idx);
            refreshLaunchpadLEDs();
        }
    }
    else if (msg.isNoteOff())
    {
        entry.kind     = MidiLogEntry::Kind::NoteOff;
        entry.channel  = static_cast<juce::uint8>(msg.getChannel());
        entry.pitch    = static_cast<juce::uint8>(msg.getNoteNumber());
        entry.velocity = static_cast<juce::uint8>(msg.getVelocity());
        midiLog.push(entry);
    }
}

// ---------------------------------------------------------------------------
// Launchpad Programmer mode + LED feedback
// ---------------------------------------------------------------------------
void SceneEngine::sendLaunchpadProgrammerMode()
{
    if (!midiOut) return;
    // SysEx: enter Programmer mode (Novation Launchpad Mini MK3)
    const uint8_t sysex[] = {
        0xF0, 0x00, 0x20, 0x29, 0x02, 0x0D, 0x0E, 0x01, 0xF7
    };
    midiOut->sendMessageNow(juce::MidiMessage(sysex, sizeof(sysex)));
}

void SceneEngine::setActiveScene(int idx) noexcept
{
    if (idx >= 0 && idx < 64)
    {
        activeScene.store(idx);
        refreshLaunchpadLEDs();
    }
}

void SceneEngine::refreshLaunchpadLEDs()
{
    if (!midiOut) return;
    const int active = activeScene.load();
    for (int i = 0; i < 64; ++i)
    {
        const auto& sc = kScenes[static_cast<size_t>(i)];
        const int row  = i / 8 + 1;  // 1-based
        const int col  = i % 8 + 1;
        const int note = row * 10 + col;
        const uint8_t vel = (i == active) ? LpColor::ActiveBrt : sc.padVelocity;
        midiOut->sendMessageNow(juce::MidiMessage::noteOn(1, note, vel));
    }
}

// ---------------------------------------------------------------------------
// 60 Hz render loop
// ---------------------------------------------------------------------------
void SceneEngine::hiResTimerCallback()
{
    const double nowSec = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    const double dt     = nowSec - lastTickSec;
    lastTickSec = nowSec;

    const int   sceneIdx = activeScene.load();
    const auto& scene    = kScenes[static_cast<size_t>(sceneIdx)];
    const float spd      = speedScale.load();
    const float tail     = tailScale.load();

    // Build target frame
    for (int barIdx = 0; barIdx < kNumBars; ++barIdx)
    {
        const auto& bar = kBars[barIdx];
        for (int pixel = 1; pixel <= bar.pixels; ++pixel)
        {
            float r, g, b;
            renderPixel(scene, nowSec, barIdx, pixel, spd, tail, r, g, b);
            const auto ch = bar.channelsFor(pixel);
            const int  i0 = ch[0] - 1;  // 0-based index into targetFrame[]
            targetFrame[i0]     = r;
            targetFrame[i0 + 1] = g;
            targetFrame[i0 + 2] = b;
        }
    }

    // LERP live toward target
    const float lerpRate = 1.0f / std::max(0.01f, crossfadeSec.load());
    const float alpha    = std::min(1.0f, static_cast<float>(dt) * lerpRate);
    for (int ch = 0; ch < kRigChannels; ++ch)
        liveFrame[ch] += (targetFrame[ch] - liveFrame[ch]) * alpha;

    // Push to DMX driver and shared DmxFrame for the GUI.
    // Global dim is applied here (post-crossfade) so the live preview and the
    // DMX output both reflect it while the crossfade stays in scene space.
    const float dim = globalDim.load();
    for (int barIdx = 0; barIdx < kNumBars; ++barIdx)
    {
        const auto& bar = kBars[barIdx];
        for (int pixel = 1; pixel <= bar.pixels; ++pixel)
        {
            const auto ch = bar.channelsFor(pixel);
            const int  i0 = ch[0] - 1;
            const auto toByte = [](float v) -> juce::uint8 {
                return static_cast<juce::uint8>(juce::jlimit(0, 255,
                    static_cast<int>(v * 255.0f + 0.5f)));
            };
            const float r = liveFrame[i0]     * dim;
            const float g = liveFrame[i0 + 1] * dim;
            const float b = liveFrame[i0 + 2] * dim;
            dmx.setChannel(ch[0], toByte(r));
            dmx.setChannel(ch[1], toByte(g));
            dmx.setChannel(ch[2], toByte(b));
            liveFrameOut.set(ch[0], r);
            liveFrameOut.set(ch[1], g);
            liveFrameOut.set(ch[2], b);
        }
    }
}

}  // namespace hitlaunchdmx
