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
    // The rig is one long vertical line of kNumLeds (72) pixels.
    // gid 0 = very bottom (bar 0, pixel 1); gid 71 = very top (bar 3, pixel 18).
    // Every positional recipe runs over this full span.
    // The rig is one long vertical line of kNumLeds (72) pixels.
    // gid 0 = very bottom (bar 0, pixel 1); gid 71 = very top (bar 3, pixel 18).
    // Most recipes run over this full span; QUAD_* run within each 18-LED bar.
    const int   nLeds     = kNumLeds;                                 // 72
    const float fLeds     = static_cast<float>(nLeds);
    const int   gid       = gidFor(barIdx, pixel);                    // 0..71
    const float gf        = static_cast<float>(gid);
    const int   nPix      = kPixelsPerBar;                            // 18 (per bar)
    const int   lp        = pixel - 1;                               // 0..17 within bar
    const int   reps      = juce::jmax(1, sc.reps);
    const float baseSpeed = sc.speed * speedScale;                    // scaled motion rate
    const float speed     = baseSpeed * static_cast<float>(sc.direction);
    const float ft        = static_cast<float>(t) * std::abs(baseSpeed);
    const float fts       = static_cast<float>(t) * speed;            // signed speed
    const float cometW    = juce::jlimit(0.02f, 1.0f, sc.width * tailScale);  // scaled tail length

    float brightness = 0.0f;
    PaletteColor col = sc.color1;

    switch (sc.motion)
    {
    // ── MOON_RISE ─────────────────────────────────────────────────────────
    // A glow that climbs the full 72-line from below the bottom to past the top.
    case MotionType::MOON_RISE:
    {
        const float phase  = std::fmod(ft, 1.0f);
        const float centre = phase * (fLeds + 16.0f) - 8.0f;         // -8 .. nLeds+8
        const float sigma  = fLeds * 0.14f;
        const float d      = gf - centre;
        brightness = std::exp(-(d * d) / (2.0f * sigma * sigma));
        col = sc.color1;
        break;
    }

    // ── DRIFT_BALL ────────────────────────────────────────────────────────
    // A soft Gaussian ball drifts smoothly up and down the full 72-line.
    case MotionType::DRIFT_BALL:
    {
        const float centre = (fLeds - 1.0f) * 0.5f * (1.0f - std::cos(kTwoPi * ft));
        const float sigma  = juce::jmax(1.0f, sc.width * fLeds * 0.35f);
        const float d      = gf - centre;
        brightness = std::exp(-(d * d) / (2.0f * sigma * sigma));
        col = sc.color1;
        break;
    }

    // ── SWEEP ───────────────────────────────────────────────────────────────
    // A wide highlight sweeps along the line, wrapping bottom<->top.
    case MotionType::SWEEP:
    {
        const float pos = std::fmod(std::fmod(fts, 1.0f) + 1.0f, 1.0f) * fLeds;
        brightness = gaussRing(gf, pos, fLeds, 0.18f);
        col = sc.color1;
        break;
    }

    // ── COMET ───────────────────────────────────────────────────────────────
    // Single comet with tail travels the full 72-line, wrapping.
    case MotionType::COMET:
    {
        const int head   = ((static_cast<int>(fts * fLeds) % nLeds) + nLeds) % nLeds;
        const int behind = behindWrapped(head, gid, nLeds, sc.direction);
        brightness = cometBrightness(behind, nLeds, cometW);
        col = sc.color1;
        break;
    }

    // ── MULTI_COMET ───────────────────────────────────────────────────────
    // `reps` comets evenly spaced around the wrapping line, same direction.
    case MotionType::MULTI_COMET:
    {
        const int head    = ((static_cast<int>(fts * fLeds) % nLeds) + nLeds) % nLeds;
        const int spacing = nLeds / reps;
        float b = 0.0f;
        for (int r = 0; r < reps; ++r)
        {
            const int h      = (head + r * spacing) % nLeds;
            const int behind = behindWrapped(h, gid, nLeds, sc.direction);
            b = juce::jmax(b, cometBrightness(behind, nLeds, cometW));
        }
        brightness = b;
        col = sc.color1;
        break;
    }

    // ── PING_PONG ───────────────────────────────────────────────────────────
    // Comet bounces up the full line then back down (no wrap; reflects at ends).
    case MotionType::PING_PONG:
    {
        const float ph = std::fmod(ft, 2.0f);                       // 0..2 cycle
        int head, dir;
        if (ph < 1.0f) { head = static_cast<int>(ph * fLeds);             dir = +1; }
        else           { head = static_cast<int>((2.0f - ph) * fLeds);   dir = -1; }
        const int behind = (dir > 0) ? (head - gid) : (gid - head);
        brightness = cometBrightness(behind, nLeds, cometW);
        col = sc.color1;
        break;
    }

    // ── QUAD_CHASE ──────────────────────────────────────────────────────────
    // A comet chases within EACH 18-LED bar simultaneously (4 chases at once).
    case MotionType::QUAD_CHASE:
    {
        const int head   = ((static_cast<int>(fts * static_cast<float>(nPix)) % nPix) + nPix) % nPix;
        const int behind = behindWrapped(head, lp, nPix, sc.direction);
        brightness = cometBrightness(behind, nPix, cometW);
        col = sc.color1;
        break;
    }

    // ── QUAD_PING_PONG ──────────────────────────────────────────────────────
    // Ping-pong within each bar; alternate bars bounce in the opposite phase
    // so the rig reads as a symmetric in/out motion.
    case MotionType::QUAD_PING_PONG:
    {
        const int   barDir = (barIdx % 2 == 0) ? sc.direction : -sc.direction;
        const float ph     = std::fmod(ft, 2.0f);
        int head, dir;
        if (ph < 1.0f) { head = static_cast<int>(ph * static_cast<float>(nPix));           dir = +1; }
        else           { head = static_cast<int>((2.0f - ph) * static_cast<float>(nPix));  dir = -1; }
        dir *= barDir;
        const int behind = (dir > 0) ? (head - lp) : (lp - head);
        brightness = cometBrightness(behind, nPix, cometW);
        col = sc.color1;
        break;
    }

    // ── ODD_EVEN ──────────────────────────────────────────────────────────
    // Shimmer crossfading between the odd and even LEDs of the whole line.
    case MotionType::ODD_EVEN:
    {
        const float phase = (1.0f + std::sin(kTwoPi * ft)) * 0.5f;
        brightness = (gid % 2 == 0) ? phase : (1.0f - phase);
        col = sc.color1;
        break;
    }

    // ── COLOUR_WASH ───────────────────────────────────────────────────────
    // Travelling colour wave between color1 and color2; `reps` spatial cycles.
    case MotionType::COLOUR_WASH:
    {
        const float mix = (1.0f + std::sin(kTwoPi * (fts + static_cast<float>(reps) * gf / fLeds))) * 0.5f;
        col = { sc.color1.r + mix * (sc.color2.r - sc.color1.r),
                sc.color1.g + mix * (sc.color2.g - sc.color1.g),
                sc.color1.b + mix * (sc.color2.b - sc.color1.b) };
        brightness = 1.0f;
        break;
    }

    // ── PULSE ───────────────────────────────────────────────────────────────
    // Travelling brightness wave along the full line; `reps` spatial cycles.
    case MotionType::PULSE:
    {
        brightness = (1.0f + std::sin(kTwoPi * (fts + static_cast<float>(reps) * gf / fLeds))) * 0.5f;
        col = sc.color1;
        break;
    }

    // ── RAINBOW ─────────────────────────────────────────────────────────────
    // Hue progresses along the full line and scrolls over time; `reps` rainbows.
    case MotionType::RAINBOW:
    {
        const float hue = std::fmod(fts + static_cast<float>(reps) * gf / fLeds + 1.0f, 1.0f);
        col = hsvToRgb(hue, 1.0f, 1.0f);
        brightness = 1.0f;
        break;
    }

    // ── WIPE ──────────────────────────────────────────────────────────────
    // Fill sweeps along the full line; colour crossfades color1->color2 with fill.
    case MotionType::WIPE:
    {
        const float phase = std::fmod(ft, 1.0f);
        const int   fill  = static_cast<int>(phase * fLeds);
        const float mix   = (fill > 0) ? static_cast<float>(fill) / fLeds : 0.0f;
        col = { sc.color1.r + mix * (sc.color2.r - sc.color1.r),
                sc.color1.g + mix * (sc.color2.g - sc.color1.g),
                sc.color1.b + mix * (sc.color2.b - sc.color1.b) };
        brightness = sc.direction > 0 ? (gid <= fill ? 1.0f : 0.0f)
                                       : (gid >= (nLeds - fill) ? 1.0f : 0.0f);
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

        const int note = msg.getNoteNumber();
        const int idx  = noteToSceneIndex(note);   // in-page 0..63
        if (idx >= 0)
        {
            // Grid pad on the currently-viewed page selects that scene.
            activeScene.store(viewPage.load() * kScenesPerPage + idx);
            refreshLaunchpadLEDs();
        }
        else
        {
            const int page = noteToPageIndex(note);  // top-row page button
            if (page >= 0)
            {
                viewPage.store(page);
                refreshLaunchpadLEDs();
            }
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

void SceneEngine::setActiveScene(int flat) noexcept
{
    if (flat >= 0 && flat < kTotalScenes)
    {
        activeScene.store(flat);
        refreshLaunchpadLEDs();
    }
}

void SceneEngine::setViewPage(int page) noexcept
{
    if (page >= 0 && page < kNumPages)
    {
        viewPage.store(page);
        refreshLaunchpadLEDs();
    }
}

void SceneEngine::refreshLaunchpadLEDs()
{
    if (!midiOut) return;
    const int active = activeScene.load();
    const int page   = viewPage.load();

    // Light each pad with the scene's EXACT colour via the Launchpad Mini MK3
    // RGB lighting SysEx (programmer mode). One message carries all pads:
    //   F0 00 20 29 02 0D 03  [spec...]  F7
    //   RGB spec = 03 <note> <r 0-127> <g 0-127> <b 0-127>
    // The grid shows the currently-viewed page; the active pad (if on this
    // page) is full brightness, the rest dimmed. The right-hand Scene Launch
    // buttons select the page (active = white, others = dim white).
    std::vector<uint8_t> msg = { 0xF0, 0x00, 0x20, 0x29, 0x02, 0x0D, 0x03 };

    for (int i = 0; i < kScenesPerPage; ++i)
    {
        const int   flat = page * kScenesPerPage + i;
        const auto& sc   = kScenes[static_cast<size_t>(flat)];
        const int   row  = i / 8 + 1;  // 1-based
        const int   col  = i % 8 + 1;
        const int   note = row * 10 + col;
        const float gain = (flat == active) ? 1.0f : 0.22f;

        const auto to7 = [gain](float c) -> uint8_t {
            return static_cast<uint8_t>(juce::jlimit(0, 127, static_cast<int>(c * gain * 127.0f + 0.5f)));
        };

        msg.push_back(0x03);
        msg.push_back(static_cast<uint8_t>(note));
        msg.push_back(to7(sc.color1.r));
        msg.push_back(to7(sc.color1.g));
        msg.push_back(to7(sc.color1.b));
    }

    // Page-selector buttons (right-hand Scene Launch column).
    for (int p = 0; p < kNumPages; ++p)
    {
        const uint8_t lvl = (p == page) ? 127 : 12;  // white: bright = current
        msg.push_back(0x03);
        msg.push_back(static_cast<uint8_t>(pageButtonNote(p)));
        msg.push_back(lvl);
        msg.push_back(lvl);
        msg.push_back(lvl);
    }

    msg.push_back(0xF7);
    midiOut->sendMessageNow(juce::MidiMessage(msg.data(), static_cast<int>(msg.size())));
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
