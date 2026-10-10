#include "Surfaces.h"

#include <cmath>

#include "AutoPrograms.h"
#include "Palette.h"             // hitnotedmx
#include "Recipes.h"             // hitnotedmx
#include "TriggerVocabulary.h"   // hitnotedmx

namespace hitlaunchdmx
{

// ─── ActivityLog ────────────────────────────────────────────────────────────

void ActivityLog::add (const juce::String& line)
{
    const juce::ScopedLock sl (lock);
    lines.add (line);
    if (lines.size() > 40)
        lines.removeRange (0, lines.size() - 40);
    dirty = true;
}

bool ActivityLog::takeIfChanged (juce::String& out)
{
    const juce::ScopedLock sl (lock);
    if (! dirty)
        return false;
    dirty = false;
    out = lines.joinIntoString ("\n");
    return true;
}

// ─── Shared descriptions ────────────────────────────────────────────────────

namespace
{
constexpr float kIdle = 0.16f;   // brightness of an unlatched pad

juce::Colour rgb (float r, float g, float b)
{
    return juce::Colour::fromFloatRGBA (r, g, b, 1.0f);
}

// Representative colour of each Multicolor recipe: its average RGB over the
// default grid and a few beats, normalised so the brightest channel is full.
const std::array<juce::Colour, hitnotedmx::kNumColorDyn>& multicolorSwatches()
{
    static const auto table = []
    {
        std::array<juce::Colour, hitnotedmx::kNumColorDyn> out;
        for (int i = 0; i < hitnotedmx::kNumColorDyn; ++i)
        {
            const auto fn = hitnotedmx::getColorRecipe (hitnotedmx::kColorDynStart + i);
            float r = 0, g = 0, b = 0;
            if (fn != nullptr)
                for (int s = 0; s < 16; ++s)
                    for (int bar = 0; bar < 4; ++bar)
                        for (int px = 1; px <= 18; px += 3)
                        {
                            const auto c = fn (s * 0.25, bar, px, 18, 4, 0.5f);
                            r += c.r; g += c.g; b += c.b;
                        }
            const float m = juce::jmax (r, g, b, 1.0e-6f);
            out[static_cast<size_t> (i)] = rgb (r / m, g / m, b / m);
        }
        return out;
    }();
    return table;
}

juce::Colour pitchColour (int p)
{
    using namespace hitnotedmx;
    if (p >= kPrimaryPaletteStart && p < kSecondaryPaletteStart)
    {
        const auto c = kPalette[static_cast<size_t> (p - kPrimaryPaletteStart)];
        return p == kPrimaryPaletteStart ? juce::Colour (0xff303030) : rgb (c.r, c.g, c.b);   // "Black" swatch
    }
    if (p >= kSecondaryPaletteStart && p < kSecondaryPaletteEnd)
    {
        const auto c = kSecondaryPalette[static_cast<size_t> (p - kSecondaryPaletteStart)];
        return rgb (c.r, c.g, c.b);
    }
    if (isColorDynPitch (p))   return multicolorSwatches()[static_cast<size_t> (p - kColorDynStart)];
    if (p == kStrobePitch)     return juce::Colours::white;
    if (isChasesPitch (p))     return rgb (0.0f, 0.75f, 1.0f);
    if (isBreathesPitch (p))   return rgb (0.45f, 0.25f, 1.0f);
    if (isWildPitch (p))       return rgb (1.0f, 0.35f, 0.0f);
    if (p >= kSpotPitch0 && p < kSpotPitch0 + 4) return (p - kSpotPitch0) % 2 == 0 ? rgb (1.0f, 0.7f, 0.35f) : rgb (0.6f, 0.4f, 1.0f);
    return juce::Colours::grey;
}

juce::String pitchLabel (int p)
{
    auto name = hitnotedmx::vocab::chainName (p);
    return name.length() > 3 ? name.substring (3) : name;
}

// XL 2-bit red/green LED code → screen colour.
juce::Colour xlCodeColour (int code)
{
    const int r = code & 3;
    const int g = (code >> 4) & 3;
    if (r == 0 && g == 0)
        return juce::Colour (0xff2a2a2a);
    return rgb (r / 3.0f, g / 3.0f, 0.0f);
}

int xlButtonCode (const LiveEngine& e, int b)
{
    const bool on = e.isButtonOn (b);
    switch (static_cast<B> (b))
    {
        case B::BumpBar1: case B::BumpBar2: case B::BumpBar3: case B::BumpBar4:
        case B::BumpBar5: case B::BumpBar6: case B::BumpBar7:
            // Dark when the grid has no such bar.
            if (b - idx (B::BumpBar1) >= e.getRig().cols) return xl::kLedOff;
            return on ? xl::kLedRed : xl::kLedRedLo;
        case B::Blackout:                    return on ? xl::kLedRed   : xl::kLedRedLo;
        case B::Strobe:                      return on || e.getStrobeHz() > 0.0f ? xl::kLedYellow : xl::kLedAmberLo;
        case B::Freeze:   case B::Flip:
        case B::Reverse:                     return on ? xl::kLedGreen : xl::kLedGreenLo;
        case B::ToBlack:  case B::FromBlack: return on ? xl::kLedAmber : xl::kLedAmberLo;
        case B::Tap:
        {
            // Blink on the beat.
            const double ph = e.getGlobalBeats() - std::floor (e.getGlobalBeats());
            return on || ph < 0.15 ? xl::kLedGreen : xl::kLedGreenLo;
        }
        case B::Restart:                     return on ? xl::kLedGreen : xl::kLedGreenLo;
        default:                             return xl::kLedOff;
    }
}

int xlKnobCode (int p)
{
    return kParamDefs[static_cast<size_t> (p)].used ? xl::kLedAmberLo : xl::kLedOff;
}
}  // namespace

PadView describeLaunchpad (const LiveEngine& e, int row, int col)
{
    const int page = e.getViewPage();

    if (row == 9 && col == 9)
        return { juce::Colour (0xffff4fa3), {}, true };   // logo: Hitmix pink

    if (col == 9)   // right column: page buttons, top = page 1
    {
        const int pg = 8 - row;   // row 8 → page 0 … row 1 → page 7
        const auto base = pg == kPageMotion ? rgb (0.0f, 0.75f, 1.0f)
                        : pg == kPageColour ? rgb (1.0f, 0.3f, 0.7f)
                                            : rgb (0.2f, 1.0f, 0.3f);
        const bool cur = pg == page;
        return { cur ? juce::Colours::white : base.withMultipliedBrightness (0.35f), kPageNames[pg], cur };
    }

    if (row == 9)   // top round buttons
    {
        const auto tb = static_cast<TopButton> (col - 1);
        bool lit = false;
        juce::Colour c;
        switch (tb)
        {
            case TopButton::SpotLWW: case TopButton::SpotLCol:
            case TopButton::SpotRWW: case TopButton::SpotRCol:
            {
                const int p = kSpotPitch0 + (col - 1);
                lit = e.getSel (p) != 0;
                c = pitchColour (p);
                break;
            }
            case TopButton::Auto1: case TopButton::Auto2:
            case TopButton::Auto3: case TopButton::Auto4:
            {
                // Mellow → Disco: teal, green, orange, magenta.
                static const juce::Colour autoCol[kNumAutoLevels] {
                    rgb (0.1f, 0.7f, 0.9f), rgb (0.3f, 1.0f, 0.2f), rgb (1.0f, 0.5f, 0.0f), rgb (1.0f, 0.1f, 0.8f)
                };
                const int lvl = col - 5;
                lit = e.getAutoLevel() == lvl;
                c = autoCol[lvl];
                if (lit && e.getAutoProgram() >= 0)
                    return { c, juce::String (kTopButtonNames[col - 1]) + " " + juce::String (e.getAutoProgram() + 1)
                                  + "/" + juce::String (autoLevels()[static_cast<size_t> (lvl)].numPrograms), true };
                break;
            }
        }
        return { lit ? c : c.withMultipliedBrightness (kIdle), kTopButtonNames[col - 1], lit };
    }

    if (const int pitch = padPitch (page, row, col); pitch >= 0)
    {
        const auto s  = e.getSel (pitch);
        const auto c  = pitchColour (pitch);
        const auto label = pitchLabel (pitch);
        return { s != 0 ? c : c.withMultipliedBrightness (kIdle), label, s != 0 };
    }

    if (const int slot = padLook (page, row, col); slot >= 0)
    {
        const bool used   = e.lookUsed (slot);
        const bool active = e.getActiveLook() == slot;
        const auto label  = "Look " + juce::String (slot + 1);
        const bool held   = e.getHeldLookSlot() == slot;
        if (active)               return { juce::Colours::lime, label, true };            // on stage / just stored
        if (held)                 return { juce::Colours::white.withMultipliedBrightness (0.5f), "hold = store", false };
        if (used)                 return { rgb (0.2f, 1.0f, 0.3f).withMultipliedBrightness (0.3f), label, false };
        return { juce::Colours::black, {}, false };
    }

    return { juce::Colours::black, {}, false };
}

PadView describeXlButton (const LiveEngine& e, int b)
{
    const auto& def = kButtonDefs[static_cast<size_t> (b)];
    return { xlCodeColour (xlButtonCode (e, b)), def.kind == ButtonKind::Spare ? juce::String() : juce::String (def.name),
             e.isButtonOn (b) };
}

// ─── Device discovery ───────────────────────────────────────────────────────

namespace
{
juce::String findDevice (const juce::Array<juce::MidiDeviceInfo>& devs,
                         std::function<bool (const juce::String&)> match)
{
    for (const auto& d : devs)
        if (match (d.name))
            return d.name;
    return {};
}

std::unique_ptr<juce::MidiInput> openIn (const juce::String& name, juce::MidiInputCallback* cb)
{
    for (const auto& d : juce::MidiInput::getAvailableDevices())
        if (d.name == name)
            return juce::MidiInput::openDevice (d.identifier, cb);
    return nullptr;
}

std::unique_ptr<juce::MidiOutput> openOut (const juce::String& name)
{
    for (const auto& d : juce::MidiOutput::getAvailableDevices())
        if (d.name == name)
            return juce::MidiOutput::openDevice (d.identifier);
    return nullptr;
}

bool isLaunchpadMidiPort (const juce::String& n)
{
    // The Mini MK3 exposes a DAW port and a MIDI port; Programmer mode lives on
    // the MIDI one.
    return (n.containsIgnoreCase ("LPMiniMK3") || n.containsIgnoreCase ("Launchpad"))
        && ! n.containsIgnoreCase ("DAW");
}

bool isXlPort (const juce::String& n) { return n.containsIgnoreCase ("Launch Control XL"); }

juce::String describeMsg (const juce::MidiMessage& m)
{
    if (m.isNoteOn())     return "note " + juce::String (m.getNoteNumber()) + " on  " + juce::String (m.getVelocity()) + "  ch" + juce::String (m.getChannel());
    if (m.isNoteOff())    return "note " + juce::String (m.getNoteNumber()) + " off     ch" + juce::String (m.getChannel());
    if (m.isController()) return "cc   " + juce::String (m.getControllerNumber()) + " = " + juce::String (m.getControllerValue()) + "  ch" + juce::String (m.getChannel());
    return m.getDescription();
}
}  // namespace

// ─── Launchpad ──────────────────────────────────────────────────────────────

LaunchpadSurface::LaunchpadSurface (LiveEngine& e, ActivityLog& l) : engine (e), log (l) {}
LaunchpadSurface::~LaunchpadSurface() { close(); }

juce::String LaunchpadSurface::findInput()  { return findDevice (juce::MidiInput::getAvailableDevices(),  isLaunchpadMidiPort); }
juce::String LaunchpadSurface::findOutput() { return findDevice (juce::MidiOutput::getAvailableDevices(), isLaunchpadMidiPort); }

bool LaunchpadSurface::open (const juce::String& inName, const juce::String& outName)
{
    close();
    midiIn  = openIn (inName, this);
    midiOut = openOut (outName);
    if (midiIn != nullptr)
    {
        midiIn->start();
        openName = inName;
    }
    if (midiOut != nullptr)
    {
        // Enter Programmer mode: pads send note 10×row+col, edge buttons CC.
        const juce::uint8 sysex[] = { 0xF0, 0x00, 0x20, 0x29, 0x02, 0x0D, 0x0E, 0x01, 0xF7 };
        midiOut->sendMessageNow (juce::MidiMessage (sysex, sizeof (sysex)));
        refreshLeds();
    }
    return midiIn != nullptr;
}

void LaunchpadSurface::close()
{
    if (midiIn != nullptr) midiIn->stop();
    midiIn.reset();
    midiOut.reset();
    openName.clear();
}

void LaunchpadSurface::handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& m)
{
    log.add ("LP  " + describeMsg (m));

    // Pads: note on = press, note off / velocity-0 note on = release (the
    // release drives look recall-vs-store and the hold-to-combine chord).
    if (m.isNoteOn())
    {
        const int n = m.getNoteNumber();
        engine.padPressed (n / 10, n % 10);
        return;
    }
    if (m.isNoteOff())
    {
        const int n = m.getNoteNumber();
        engine.padReleased (n / 10, n % 10);
        return;
    }
    if (! m.isController())
        return;

    const int cc   = m.getControllerNumber();
    const bool down = m.getControllerValue() > 0;
    if (cc >= 91 && cc <= 98)
        engine.topButton (static_cast<TopButton> (cc - 91), down);
    else if (cc % 10 == 9 && cc >= 19 && cc <= 89)
        engine.pageButton (8 - cc / 10, down);   // hold 1 s = clear
}

void LaunchpadSurface::refreshLeds()
{
    if (midiOut == nullptr)
        return;

    // One RGB lighting SysEx for every LED: F0 00 20 29 02 0D 03 {03 id r g b}… F7
    std::vector<juce::uint8> msg { 0xF0, 0x00, 0x20, 0x29, 0x02, 0x0D, 0x03 };
    const auto to7 = [] (juce::uint8 v) { return static_cast<juce::uint8> (v >> 1); };

    for (int row = 1; row <= 9; ++row)
        for (int col = 1; col <= 9; ++col)
        {
            const auto view = describeLaunchpad (engine, row, col);
            msg.push_back (0x03);
            msg.push_back (static_cast<juce::uint8> (row * 10 + col));
            msg.push_back (to7 (view.colour.getRed()));
            msg.push_back (to7 (view.colour.getGreen()));
            msg.push_back (to7 (view.colour.getBlue()));
        }
    msg.push_back (0xF7);
    midiOut->sendMessageNow (juce::MidiMessage (msg.data(), static_cast<int> (msg.size())));
}

// ─── Launch Control XL ──────────────────────────────────────────────────────

LaunchControlXlSurface::LaunchControlXlSurface (LiveEngine& e, ActivityLog& l) : engine (e), log (l) {}
LaunchControlXlSurface::~LaunchControlXlSurface() { close(); }

juce::String LaunchControlXlSurface::findInput()  { return findDevice (juce::MidiInput::getAvailableDevices(),  isXlPort); }
juce::String LaunchControlXlSurface::findOutput() { return findDevice (juce::MidiOutput::getAvailableDevices(), isXlPort); }

bool LaunchControlXlSurface::open (const juce::String& inName, const juce::String& outName)
{
    close();
    midiIn  = openIn (inName, this);
    midiOut = openOut (outName);
    if (midiIn != nullptr)
    {
        midiIn->start();
        openName = inName;
    }
    refreshLeds();
    return midiIn != nullptr;
}

void LaunchControlXlSurface::close()
{
    if (midiIn != nullptr) midiIn->stop();
    midiIn.reset();
    midiOut.reset();
    openName.clear();
}

void LaunchControlXlSurface::handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& m)
{
    log.add ("XL  " + describeMsg (m));

    // Factory template 1 talks on channel 9; other templates are ignored so a
    // template switch on the unit can't scramble the mapping.
    if (m.getChannel() != xl::kChannel)
        return;

    if (m.isController())
    {
        engine.setParam (xl::paramForCc (m.getControllerNumber()), m.getControllerValue() / 127.0f);
        return;
    }
    if (m.isNoteOnOrOff())
        engine.buttonEvent (xl::buttonForNote (m.getNoteNumber()), m.isNoteOn());
}

void LaunchControlXlSurface::refreshLeds()
{
    if (midiOut == nullptr)
        return;

    // Set LEDs SysEx: F0 00 20 29 02 11 78 <template> {<index> <value>}… F7
    // index 0-23 knobs (row-major), 24-31 focus buttons, 32-39 control buttons.
    std::vector<juce::uint8> msg { 0xF0, 0x00, 0x20, 0x29, 0x02, 0x11, 0x78,
                                   static_cast<juce::uint8> (xl::kTemplateIndex) };
    for (int k = 0; k < kFaderParam0; ++k)
    {
        msg.push_back (static_cast<juce::uint8> (k));
        msg.push_back (static_cast<juce::uint8> (xlKnobCode (k)));
    }
    for (int b = 0; b < kNumButtons; ++b)
    {
        msg.push_back (static_cast<juce::uint8> (24 + b));
        msg.push_back (static_cast<juce::uint8> (xlButtonCode (engine, b)));
    }
    msg.push_back (0xF7);
    midiOut->sendMessageNow (juce::MidiMessage (msg.data(), static_cast<int> (msg.size())));
}

}  // namespace hitlaunchdmx
