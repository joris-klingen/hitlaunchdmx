#include "LiveEngine.h"

#include <cmath>

#include "AutoPrograms.h"
#include "Recipes.h"   // hitnotedmx: bank ranges, strobe constants

namespace hitlaunchdmx
{

namespace
{
constexpr int kTickHz = 100;

// "127 = instant … 1 = slowest" velocities, from a knob where 0 = instant.
int instantVel (float n) noexcept { return juce::jlimit (1, 127, 127 - juce::roundToInt (n * 126.0f)); }
}  // namespace

LiveEngine::LiveEngine (EnttecProDmx& d) : dmx (d)
{
    for (int i = 0; i < kNumParams; ++i)
        params[static_cast<size_t> (i)].store (kParamDefs[static_cast<size_t> (i)].deflt);
    for (auto& b : buttonOn)      b.store (false);
    for (auto& b : buttonPresses) b.store (0);
    for (auto& s : sel)           s.store (0);
    for (auto& d : padDown)       d.store (false);
}

LiveEngine::~LiveEngine() { stop(); }

void LiveEngine::start()
{
    lastTickSec = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    startTimer (1000 / kTickHz);
}

void LiveEngine::stop() { stopTimer(); }

// ─── XL ─────────────────────────────────────────────────────────────────────

void LiveEngine::setParam (int p, float norm) noexcept
{
    if (p < 0 || p >= kNumParams)
        return;
    params[static_cast<size_t> (p)].store (juce::jlimit (0.0f, 1.0f, norm));
    changed (false);
}

void LiveEngine::buttonEvent (int b, bool down) noexcept
{
    if (b < 0 || b >= kNumButtons)
        return;
    if (b == idx (B::Blackout))
    {
        if (down) blackoutOn.store (! blackoutOn.load());
        changed();
        return;
    }
    const auto kind = kButtonDefs[static_cast<size_t> (b)].kind;
    auto& on = buttonOn[static_cast<size_t> (b)];

    switch (kind)
    {
        case ButtonKind::Momentary:
            on.store (down);
            if (down) buttonPresses[static_cast<size_t> (b)].fetch_add (1);
            break;
        case ButtonKind::Toggle:
            if (down) on.store (! on.load());
            break;
        case ButtonKind::Trigger:
            on.store (down);                   // lit while pressed
            if (down)
            {
                if (b == idx (B::Tap))     tap();
                if (b == idx (B::Restart)) restartClock();
            }
            break;
        case ButtonKind::Spare:
            return;
    }
    changed();
}

// ─── Launchpad ──────────────────────────────────────────────────────────────

void LiveEngine::setViewPage (int page) noexcept
{
    if (page >= 0 && page < kNumPages)
    {
        viewPage.store (page);
        changed();
    }
}

void LiveEngine::latchPitch (int pitch) noexcept
{
    auto& s = sel[static_cast<size_t> (pitch)];
    const auto cur = s.load();
    const std::uint8_t next = cur == 0 ? 127 : 0;

    // A NEW pad replaces the rest of its group (one chase, one primary, …),
    // unless another pad of the group is being held (or on-screen Shift) —
    // then it is ADDED, so recipes can combine.
    const int group = pitchGroup (pitch);
    if (cur == 0 && group >= 0 && ! shiftHeld.load())
    {
        bool chord = false;
        for (int p = 0; p < 128 && ! chord; ++p)
            chord = p != pitch && pitchGroup (p) == group && padDown[static_cast<size_t> (p)].load();
        if (! chord)
            for (int p = 0; p < 128; ++p)
                if (p != pitch && pitchGroup (p) == group)
                    sel[static_cast<size_t> (p)].store (0);
    }

    s.store (next);
    activeLook.store (-1);   // the look on stage is now a modified one
    stopAutoByHand();
}

void LiveEngine::recallLook (int slot) noexcept
{
    const auto& look = looks[static_cast<size_t> (slot)];
    if (! look.used)
        return;
    for (int p = 0; p < 128; ++p)
        sel[static_cast<size_t> (p)].store (look.sel[static_cast<size_t> (p)]);
    activeLook.store (slot);
    stopAutoByHand();
}

void LiveEngine::storeLook (int slot) noexcept
{
    auto& look = looks[static_cast<size_t> (slot)];
    look.used = true;
    for (int p = 0; p < 128; ++p)
        look.sel[static_cast<size_t> (p)] = sel[static_cast<size_t> (p)].load();
    activeLook.store (slot);
}

void LiveEngine::padPressed (int row, int col, double nowSec) noexcept
{
    const int page = viewPage.load();
    {
        const juce::SpinLock::ScopedLockType sl (stateLock);
        if (const int pitch = padPitch (page, row, col); pitch >= 0)
        {
            latchPitch (pitch);
            padDown[static_cast<size_t> (pitch)].store (true);
        }
        else if (const int slot = padLook (page, row, col); slot >= 0)
        {
            heldLookSlot   = slot;     // recall on release, store on a long hold
            heldLookSince  = nowSec;
            heldLookStored = false;
            heldLookOut.store (slot);
        }
        else
            return;
    }
    changed();
}

void LiveEngine::padReleased (int row, int col, double nowSec) noexcept
{
    juce::ignoreUnused (nowSec);
    const int page = viewPage.load();
    {
        const juce::SpinLock::ScopedLockType sl (stateLock);
        if (const int pitch = padPitch (page, row, col); pitch >= 0)
        {
            padDown[static_cast<size_t> (pitch)].store (false);
            return;
        }
        const int slot = padLook (page, row, col);
        if (slot < 0 || slot != heldLookSlot)
            return;
        if (! heldLookStored)
            recallLook (slot);         // short tap = recall
        heldLookSlot = -1;
        heldLookOut.store (-1);
    }
    changed();
}

void LiveEngine::pageButton (int page, bool down, double nowSec) noexcept
{
    if (page < 0 || page >= kNumPages)
        return;
    {
        const juce::SpinLock::ScopedLockType sl (stateLock);
        if (down)
        {
            viewPage.store (page);
            heldPage = page;  heldPageSince = nowSec;  heldPageFired = false;
            // Changing page releases any held pad (its release lands elsewhere).
            for (auto& d : padDown) d.store (false);
            heldLookSlot = -1;  heldLookOut.store (-1);
        }
        else if (page == heldPage)
        {
            heldPage = -1;
        }
    }
    changed();
}

void LiveEngine::pollHolds (double nowSec) noexcept
{
    bool fired = false;
    bool clear = false;
    {
        const juce::SpinLock::ScopedLockType sl (stateLock);
        if (heldLookSlot >= 0 && ! heldLookStored && nowSec - heldLookSince >= kLookHoldSec)
        {
            storeLook (heldLookSlot);   // long hold = store what's on stage
            heldLookStored = true;
            fired = true;
        }
        if (heldPage >= 0 && ! heldPageFired && nowSec - heldPageSince >= kClearHoldSec)
        {
            heldPageFired = true;
            clear = true;
        }
    }
    if (clear)
        clearSelection();
    else if (fired)
        changed();
}

void LiveEngine::deleteLook (int slot) noexcept
{
    if (slot < 0 || slot >= kNumLooks)
        return;
    {
        const juce::SpinLock::ScopedLockType sl (stateLock);
        looks[static_cast<size_t> (slot)] = Look {};
        if (activeLook.load() == slot)
            activeLook.store (-1);
    }
    changed();
}

void LiveEngine::setAuto (int level) noexcept
{
    if (level < 0 || level >= kNumAutoLevels || autoLevel.load() == level)
        autoLevel.store (-1);          // stop; the current program stays on stage
    else
    {
        // Starting an auto = a clean slate: every modifier that would bend
        // the programs goes back to neutral (the selection itself is replaced
        // by the first program in the tick; spots stay as they are).
        for (const B b : { B::Freeze, B::Flip, B::Reverse, B::ToBlack, B::FromBlack })
            buttonOn[static_cast<size_t> (idx (b))].store (false);
        blackoutOn.store (false);
        params[static_cast<size_t> (idx (P::Spread))].store (0.0f);      // no bar de-sync
        params[static_cast<size_t> (idx (P::RelSpeed))].store (0.5f);    // 1×
        params[static_cast<size_t> (idx (P::Crossfade))].store (0.0f);   // auto handles transitions
        params[static_cast<size_t> (idx (P::ColorFade))].store (0.0f);
        // Recipe knobs back to their defaults so the programs look as designed
        // (a near-zero Chase tail turns every chase into hopping single pixels).
        for (const P p : { P::ChaseTail, P::BreatheDepth, P::WildDiv, P::ColorSpeed })
            params[static_cast<size_t> (idx (p))].store (kParamDefs[static_cast<size_t> (idx (p))].deflt);
        autoLevel.store (level);
        autoRestart.store (true);
    }
    changed();
}

void LiveEngine::topButton (TopButton b, bool down) noexcept
{
    switch (b)
    {
        case TopButton::SpotLWW: case TopButton::SpotLCol:
        case TopButton::SpotRWW: case TopButton::SpotRCol:
            if (down)
            {
                auto& s = sel[static_cast<size_t> (kSpotPitch0 + static_cast<int> (b))];
                s.store (s.load() ? 0 : 127);
                activeLook.store (-1);
            }
            break;
        case TopButton::Auto1: case TopButton::Auto2:
        case TopButton::Auto3: case TopButton::Auto4:
            if (down)
                setAuto (static_cast<int> (b) - static_cast<int> (TopButton::Auto1));
            return;
    }
    changed();
}

void LiveEngine::clearSelection() noexcept
{
    {
        const juce::SpinLock::ScopedLockType sl (stateLock);
        for (auto& s : sel) s.store (0);
        activeLook.store (-1);
        stopAutoByHand();
    }
    changed();
}

bool LiveEngine::lookUsed (int slot) const
{
    if (slot < 0 || slot >= kNumLooks)
        return false;
    const juce::SpinLock::ScopedLockType sl (stateLock);
    return looks[static_cast<size_t> (slot)].used;
}

// ─── Tempo ──────────────────────────────────────────────────────────────────

void LiveEngine::tap() noexcept
{
    const double now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    {
        const juce::SpinLock::ScopedLockType sl (stateLock);
        const double gap = now - lastTapSec;
        lastTapSec = now;

        // A gap outside the tempo range starts a fresh tap sequence.
        if (gap < 60.0 / kMaxBpm * 0.8 || gap > 60.0 / kMinBpm * 1.25)
        {
            numTapIntervals = 0;
        }
        else
        {
            // Rolling window of the last 4 intervals.
            for (int i = static_cast<int> (tapIntervals.size()) - 1; i > 0; --i)
                tapIntervals[static_cast<size_t> (i)] = tapIntervals[static_cast<size_t> (i - 1)];
            tapIntervals[0] = gap;
            numTapIntervals = juce::jmin (numTapIntervals + 1, static_cast<int> (tapIntervals.size()));

            double sum = 0.0;
            for (int i = 0; i < numTapIntervals; ++i)
                sum += tapIntervals[static_cast<size_t> (i)];
            params[static_cast<size_t> (idx (P::Bpm))].store (normFromBpm (60.0 * numTapIntervals / sum));
        }
    }
    nudgeRequest.store (true);   // every tap also lands the beat on the tap
    changed();
}

// ─── Grid ───────────────────────────────────────────────────────────────────

Rig LiveEngine::setGridShape (int cols, int rows)
{
    cols = juce::jlimit (1, hitnotedmx::kMaxBars, cols);
    rows = juce::jlimit (1, hitnotedmx::kMaxRows, rows);
    while (cols * rows > hitnotedmx::kMaxGridCells && rows > 1)
        --rows;
    gridRequest.store (packGrid (cols, rows));
    changed();
    return { cols, rows };
}

// ─── The tick ───────────────────────────────────────────────────────────────

void LiveEngine::hiResTimerCallback()
{
    tick (juce::Time::getMillisecondCounterHiRes() / 1000.0);
}

void LiveEngine::tick (double nowSec)
{
    const double dt     = juce::jlimit (0.0, 0.1, nowSec - lastTickSec);
    lastTickSec = nowSec;

    // 0. Grid reshape (same as hitnotedmx's processBlock step 0).
    if (const auto req = gridRequest.load (std::memory_order_relaxed); req != appliedGrid)
    {
        grid.rig = { static_cast<int> (req >> 8), static_cast<int> (req & 0xffu) };
        grid.rebuild();
        bumpState.reset();
        dmxValues.clear();
        for (int ch = 1; ch <= hitnotedmx::kDmxUniverseSize; ++ch)
            dmx.setChannel (ch, 0);
        appliedGrid = req;
    }

    const auto prm = [this] (P p) { return params[static_cast<size_t> (idx (p))].load (std::memory_order_relaxed); };

    // 1. Clock. Global beats at the BPM; the scaled clock integrates
    //    global × relative speed, switching multiplier only ON a global beat.
    if (restartRequest.exchange (false))
    {
        globalBeats = scaledBeats = 0.0;
        bumpState.resyncClocks();
    }
    if (nudgeRequest.exchange (false))
    {
        const double shift = std::round (globalBeats) - globalBeats;
        globalBeats += shift;
        scaledBeats += shift * curMult;
        if (shift < 0.0)
            bumpState.resyncClocks();   // computeDmx's phase clocks must not see time run backwards
    }

    const double dBeats    = dt * getBpm() / 60.0;
    const int    targetStep = relSpeedStep (prm (P::RelSpeed));
    const double prevBeats  = globalBeats;
    globalBeats += dBeats;
    if (targetStep != curStep && std::floor (globalBeats) > std::floor (prevBeats))
    {
        const double boundary = std::floor (globalBeats);
        const double targetMul = relSpeedMult (targetStep);
        scaledBeats += (boundary - prevBeats) * curMult + (globalBeats - boundary) * targetMul;
        curMult = targetMul;
        curStep = targetStep;
    }
    else
    {
        scaledBeats += dBeats * curMult;
    }
    const double t = scaledBeats;

    // 1b. Auto mode: on the global bar grid, swap the look for the level's
    //     next program. Writing the selection needs stateLock, which the tick
    //     must never wait on — so try it, and retry next tick (10 ms) if busy.
    const int autoLvl = autoLevel.load (std::memory_order_relaxed);
    if (autoLvl >= 0)
    {
        const auto& L = autoLevels()[static_cast<size_t> (autoLvl)];
        if (autoRestart.exchange (false) || autoLvl != autoAppliedLevel)
        {
            autoStartBar     = std::floor (globalBeats / 4.0);
            autoAppliedLevel = autoLvl;
            autoApplied      = -1;
            autoLastChange   = -1.0e9;
        }
        const double sinceStart = std::floor (globalBeats / 4.0) - autoStartBar;      // bars
        const int    block      = static_cast<int> (sinceStart) / L.barsPerProgram;
        const int    prog       = block % L.numPrograms;
        autoNextChange = (autoStartBar + (block + 1) * L.barsPerProgram) * 4.0;

        if (prog != autoApplied)
        {
            const juce::SpinLock::ScopedTryLockType tl (stateLock);
            if (tl.isLocked())
            {
                for (int p = 0; p < 128; ++p)
                    if (p < kSpotPitch0 || p >= kSpotPitch0 + 4)   // auto leaves the spots alone
                        sel[static_cast<size_t> (p)].store (0);
                for (const int n : L.programs[static_cast<size_t> (prog)].notes)
                    if (n >= 0 && n < 128)
                        sel[static_cast<size_t> (n)].store (127);
                activeLook.store (-1);
                if (autoApplied >= 0)
                    autoLastChange = globalBeats;
                autoApplied = prog;
                autoProgramOut.store (prog);
                changed();
            }
        }
    }
    else
    {
        autoAppliedLevel = -1;
        autoApplied      = -1;
        autoProgramOut.store (-1, std::memory_order_relaxed);
    }

    // 2. Build the held-note set. A pitch keeps its onset beat while it stays
    //    held (velocity changes don't retrigger); momentary presses that both
    //    started and ended between two ticks still count as one onset.
    midiState.clear();
    std::array<bool, kNumButtons> pressedSinceLast {};
    for (int b = 0; b < kNumButtons; ++b)
    {
        const auto n = buttonPresses[static_cast<size_t> (b)].load (std::memory_order_relaxed);
        pressedSinceLast[static_cast<size_t> (b)] = n != seenPresses[static_cast<size_t> (b)];
        seenPresses[static_cast<size_t> (b)] = n;
    }
    const auto btn = [&] (B b) { return buttonOn[static_cast<size_t> (idx (b))].load (std::memory_order_relaxed); };

    std::array<int, 128> vel {};   // 0 = not held this tick
    bool retrigger[128] {};

    // 2a. Launchpad-latched look: velocity = the XL knob for that bank.
    for (int p = 0; p < 128; ++p)
    {
        const auto s = sel[static_cast<size_t> (p)].load (std::memory_order_relaxed);
        if (s == 0) continue;

        int v = 127;
        if (p == hitnotedmx::kStrobePitch)              v = velFromNorm (prm (P::StrobeRate));
        else if (hitnotedmx::isChasesPitch (p))         v = velFromNorm (prm (P::ChaseTail));
        else if (hitnotedmx::isBreathesPitch (p))       v = velFromNorm (prm (P::BreatheDepth));
        else if (hitnotedmx::isWildPitch (p))           v = velFromNorm (prm (P::WildDiv));
        else if (hitnotedmx::isColorDynPitch (p))       v = velFromNorm (prm (P::ColorSpeed));
        else if (p >= hitnotedmx::kPrimaryPaletteStart && p < hitnotedmx::kSecondaryPaletteEnd)
        {
            v = instantVel (prm (P::ColorFade));
            if (autoLvl >= 0)                          // auto: colours glide at least this slowly
                v = juce::jmin (v, autoLevels()[static_cast<size_t> (autoLvl)].colourVel);
        }
        vel[static_cast<size_t> (p)] = v;
    }

    // 2b. XL buttons + knobs that are notes in hitnotedmx.
    const bool strobeOn = btn (B::Strobe) || pressedSinceLast[static_cast<size_t> (idx (B::Strobe))]
                       || sel[hitnotedmx::kStrobePitch].load (std::memory_order_relaxed) != 0;
    if (strobeOn)
        vel[hitnotedmx::kStrobePitch] = velFromNorm (prm (P::StrobeRate));
    if (btn (B::Freeze))    vel[kFreezePitch]    = 127;
    if (btn (B::Flip))      vel[kFlipPitch]      = 127;
    if (btn (B::Reverse))   vel[kReversePitch]   = 127;
    const bool blackout = blackoutOn.load (std::memory_order_relaxed);
    if (blackout)           vel[kBlackoutPitch]  = 127;
    if (btn (B::ToBlack))   vel[kToBlackPitch]   = instantVel (prm (P::BlackGlide));
    if (btn (B::FromBlack)) vel[kFromBlackPitch] = juce::jmax (2, instantVel (prm (P::BlackGlide)));  // 1 = "hold black" sentinel

    vel[kReleasePitch] = velFromNorm (prm (P::BumpRelease));
    if (prm (P::Crossfade) > 0.0f) vel[kCrossfadePitch] = velFromNorm (prm (P::Crossfade));
    else if (autoLvl >= 0)
    {
        // Auto transitions: hold Crossfade from just BEFORE each change (the
        // xfade buffer must already be tracking the old look when the new one
        // lands) until one bar after it, so the motion isn't blurred between.
        const auto& L = autoLevels()[static_cast<size_t> (autoLvl)];
        if (L.xfadeVel > 0 && (globalBeats >= autoNextChange - 0.25 || globalBeats < autoLastChange + 4.0))
            vel[kCrossfadePitch] = L.xfadeVel;
    }
    if (prm (P::Spread)    > 0.0f) vel[kSpreadPitch]    = velFromNorm (prm (P::Spread));

    for (int p = 0; p < 128; ++p)
    {
        const int v = vel[static_cast<size_t> (p)];
        auto& on = wasOn[static_cast<size_t> (p)];
        if (v == 0) { on = false; continue; }
        if (! on || retrigger[p])
            startBeat[static_cast<size_t> (p)] = t;
        on = true;
        midiState.noteOn (static_cast<std::uint8_t> (p), 1, static_cast<std::uint8_t> (v),
                          startBeat[static_cast<size_t> (p)]);
    }

    // 3. Compose with hitnotedmx's engine.
    float barDim[hitnotedmx::kMaxBars];
    for (int i = 0; i < hitnotedmx::kMaxBars; ++i)
        barDim[i] = i < kNumBarFaders ? params[static_cast<size_t> (kFaderParam0 + i)].load (std::memory_order_relaxed) : 1.0f;

    hitnotedmx::computeDmx (midiState, t, dmxValues, grid,
                            prm (P::LedMaster), prm (P::SpotMaster),
                            &colorFade, dt, prm (P::Density),
                            &selection, &bumpState, barDim);

    // 4. Per-bar bumps — hitnotedmx's Bump flash (advanceBumpFlash /
    //    applyBumpOverlay), one bar each. Applied to a COPY of the composed
    //    frame: under Freeze computeDmx keeps dmxValues as-is, so writing the
    //    flash into it would bake the flash into the frozen look.
    outFrame = dmxValues;
    {
        const double dT = lastFlashT >= 0.0 ? juce::jmax (0.0, t - lastFlashT) : 0.0;
        lastFlashT = t;
        const float relBeats = 0.25f * std::pow (16.0f, static_cast<float> (vel[kReleasePitch] - 1) / 126.0f);
        const float rate     = static_cast<float> (dT) / relBeats;

        // Target: the most recently started held primary (hitnotedmx's
        // pickColor — ties go to the lower note), else white.
        float colR = 1.0f, colG = 1.0f, colB = 1.0f;
        double newest = -1.0;
        for (int p = hitnotedmx::kPrimaryPaletteStart; p < hitnotedmx::kSecondaryPaletteStart; ++p)
            if (vel[static_cast<size_t> (p)] > 0 && startBeat[static_cast<size_t> (p)] > newest)
            {
                newest = startBeat[static_cast<size_t> (p)];
                const auto c = hitnotedmx::kPalette[static_cast<size_t> (p - hitnotedmx::kPrimaryPaletteStart)];
                colR = c.r; colG = c.g; colB = c.b;
            }

        const float ledDim = prm (P::LedMaster);
        const float kCut   = bumpState.blackLevel;   // to/from-black dip, as hitnotedmx applies it
        for (int b = 0; b < kNumBarBumps; ++b)
        {
            auto& level = barFlashLevel[static_cast<size_t> (b)];
            if (pressedSinceLast[static_cast<size_t> (idx (B::BumpBar1) + b)] )
            {
                level = 1.0f;                                         // instant attack
                barFlashAmount[static_cast<size_t> (b)] = velFromNorm (prm (P::BumpLevel)) / 127.0f;
            }
            else if (level > 0.0f)
            {
                level = juce::jmax (0.0f, level - rate);             // zero sustain
            }

            if (level <= 0.0f || blackout || b >= grid.rig.cols)
                continue;
            const float cov = level;
            const float bri = barFlashAmount[static_cast<size_t> (b)] * (1.0f - kCut);
            const float mul = ledDim * barDim[b];
            for (int px = 1; px <= grid.rig.rows; ++px)
            {
                const auto ch = grid.rig.channelsFor (b, px);
                outFrame.set (ch[0], outFrame.get (ch[0]) * (1.0f - cov) + colR * mul * bri * cov);
                outFrame.set (ch[1], outFrame.get (ch[1]) * (1.0f - cov) + colG * mul * bri * cov);
                outFrame.set (ch[2], outFrame.get (ch[2]) * (1.0f - cov) + colB * mul * bri * cov);
            }
        }
    }

    // 5. Strobe shutter lives in the driver (as in hitnotedmx).
    float strobeHz = 0.0f;
    if (strobeOn)
        strobeHz = static_cast<float> (hitnotedmx::kStrobeMinHz
                 + (hitnotedmx::kStrobeMaxHz - hitnotedmx::kStrobeMinHz) * vel[hitnotedmx::kStrobePitch] / 127.0);
    dmx.setStrobeHz (strobeHz);
    strobeHzOut.store (strobeHz, std::memory_order_relaxed);

    // 6. Push the rig channels.
    const auto* v = outFrame.raw();
    for (int ch = 1; ch <= grid.rig.rigChannels(); ++ch)
        dmx.setChannel (ch, static_cast<juce::uint8> (juce::jlimit (0, 255, static_cast<int> (v[ch - 1] * 255.0f + 0.5f))));

    // 7. Publish finished frames for the GUI (never the mid-compose buffer).
    publishedValues    = outFrame;
    publishedSelection = selection;
    publishedBeat.store (globalBeats, std::memory_order_relaxed);
    appliedMult.store (curMult, std::memory_order_relaxed);
}

// ─── Persistence ────────────────────────────────────────────────────────────

std::unique_ptr<juce::XmlElement> LiveEngine::toXml() const
{
    auto xml = std::make_unique<juce::XmlElement> ("HitLaunchDmx");
    const auto rig = getRig();
    xml->setAttribute ("gridCols", rig.cols);
    xml->setAttribute ("gridRows", rig.rows);

    juce::StringArray ps;
    for (int i = 0; i < kNumParams; ++i)
        ps.add (juce::String (getParam (i), 4));
    xml->setAttribute ("params", ps.joinIntoString (" "));
    xml->setAttribute ("paramLayout", 2);

    const auto selToString = [] (auto get)
    {
        juce::StringArray held;
        for (int p = 0; p < 128; ++p)
            if (const int s = get (p); s != 0)
                held.add (juce::String (p) + ":" + juce::String (s));
        return held.joinIntoString (" ");
    };

    xml->setAttribute ("sel", selToString ([this] (int p) { return static_cast<int> (getSel (p)); }));

    auto* looksXml = xml->createNewChildElement ("Looks");
    const juce::SpinLock::ScopedLockType sl (stateLock);
    for (int i = 0; i < kNumLooks; ++i)
    {
        const auto& look = looks[static_cast<size_t> (i)];
        if (! look.used) continue;
        auto* e = looksXml->createNewChildElement ("Look");
        e->setAttribute ("slot", i);
        e->setAttribute ("sel", selToString ([&look] (int p) { return static_cast<int> (look.sel[static_cast<size_t> (p)]); }));
    }
    return xml;
}

void LiveEngine::fromXml (const juce::XmlElement& xml)
{
    setGridShape (xml.getIntAttribute ("gridCols", hitnotedmx::kDefaultBars),
                  xml.getIntAttribute ("gridRows", hitnotedmx::kDefaultRows));

    const auto ps = juce::StringArray::fromTokens (xml.getStringAttribute ("params"), " ", {});
    if (xml.getIntAttribute ("paramLayout", 1) >= 2)
    {
        for (int i = 0; i < juce::jmin (kNumParams, ps.size()); ++i)
            setParam (i, ps[i].getFloatValue());
    }
    else
    {
        // Layout 1 (before the master column): Strobe Hz was Send B 5 (12),
        // LED dim Pan 1 (16), Spot dim Pan 2 (17), fader 8 = bar 8 dim (31).
        for (int i = 0; i < juce::jmin (kNumParams, ps.size()); ++i)
        {
            int to = i;
            if      (i == 12) to = idx (P::StrobeRate);
            else if (i == 16) to = idx (P::LedMaster);
            else if (i == 17) to = idx (P::SpotMaster);
            else if (i == 14 || i == 15 || (i >= 18 && i <= 23) || i == 31)
                continue;   // layout-1 spares (Send B 7-8, Pan 3-8) and the bar-8 dim
            setParam (to, ps[i].getFloatValue());
        }
    }

    const auto parseSel = [] (const juce::String& s, auto set)
    {
        for (const auto& tok : juce::StringArray::fromTokens (s, " ", {}))
        {
            const int p = tok.upToFirstOccurrenceOf (":", false, false).getIntValue();
            const int v = tok.fromFirstOccurrenceOf (":", false, false).getIntValue();
            // Bar selectors (5-8) and zones (12-23) are not on the Launchpad:
            // drop them from old saved state so nothing is held that no pad
            // can release (a held bar selector would darken the other bars).
            // Blackout (0) is the XL/engine toggle, never a latched note.
            const bool unreachable = p == 0 || (p >= 5 && p <= 8) || (p >= 12 && p < 24);
            if (p >= 0 && p < 128 && v > 0 && v < 128 && ! unreachable)
                set (p, static_cast<std::uint8_t> (v));
        }
    };

    const juce::SpinLock::ScopedLockType sl (stateLock);
    for (auto& s : sel) s.store (0);
    parseSel (xml.getStringAttribute ("sel"), [this] (int p, std::uint8_t v) { sel[static_cast<size_t> (p)].store (v); });

    for (auto& l : looks) l = Look {};
    if (auto* looksXml = xml.getChildByName ("Looks"))
        for (auto* e : looksXml->getChildIterator())
        {
            const int slot = e->getIntAttribute ("slot", -1);
            if (slot < 0 || slot >= kNumLooks) continue;
            auto& look = looks[static_cast<size_t> (slot)];
            look.used = true;
            parseSel (e->getStringAttribute ("sel"), [&look] (int p, std::uint8_t v) { look.sel[static_cast<size_t> (p)] = v; });
        }
    changed();
}

}  // namespace hitlaunchdmx
