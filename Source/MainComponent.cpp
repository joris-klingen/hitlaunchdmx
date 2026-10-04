#include "MainComponent.h"

#include <cmath>

namespace hitlaunchdmx
{

namespace
{
const juce::Colour kBg       { 0xff1e1e1e };
const juce::Colour kPanel    { 0xff262626 };
const juce::Colour kText     { 0xffd8d8d8 };
const juce::Colour kDimText  { 0xff8a8a8a };
const juce::Colour kPink     { 0xffff4fa3 };

// Knob accent per XL row (matches the XL's own knob-row LED colours loosely).
const juce::Colour kRowAccent[4] { juce::Colour (0xfff2a93b), juce::Colour (0xff39c6c0),
                                   juce::Colour (0xff8ad13b), juce::Colour (0xffc06bd1) };
}  // namespace

// ============================================================================
// DimKnobLookAndFeel (adapted from hitnotedmx)
// ============================================================================
void DimKnobLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width,
                                           int height, float sliderPos,
                                           float rotaryStartAngle, float rotaryEndAngle,
                                           juce::Slider& s)
{
    auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                          static_cast<float> (width), static_cast<float> (height))
                      .reduced (3.0f);
    const auto  centre = bounds.getCentre();
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    const float lineW  = juce::jmax (3.0f, radius * 0.18f);
    const float arcR   = radius - lineW * 0.5f;
    const float angle  = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto  accent = s.isEnabled() ? s.findColour (juce::Slider::rotarySliderFillColourId)
                                       : juce::Colour (0xff444444);

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (0xff3a3a3a));
    g.strokePath (track, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (sliderPos > 0.0f && s.isEnabled())
    {
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, rotaryStartAngle, angle, true);
        g.setColour (accent);
        g.strokePath (value, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    const float bodyR = radius - lineW - 2.0f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff343434), centre.x, centre.y - bodyR,
                                             juce::Colour (0xff202020), centre.x, centre.y + bodyR, false));
    g.fillEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    g.setColour (accent.withAlpha (0.5f));
    g.drawEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

    if (! s.isEnabled())
        return;

    juce::Path pointer;
    const float thick = juce::jmax (2.0f, bodyR * 0.14f);
    const float len   = bodyR * 0.82f;
    pointer.addRoundedRectangle (-thick * 0.5f, -len, thick, len * 0.62f, thick * 0.5f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
    g.setColour (juce::Colours::white);
    g.fillPath (pointer);
}

// ============================================================================
// VirtualLaunchpad
// ============================================================================
juce::Rectangle<float> VirtualLaunchpad::cellBounds (int row, int col) const
{
    // 9 × 9 lattice: visual row 0 = top round buttons (row 9), visual col 8 =
    // the page column (col 9). A small gutter separates the edge buttons.
    const float side   = static_cast<float> (juce::jmin (getWidth(), getHeight()));
    const float gutter = side * 0.02f;
    const float cell   = (side - gutter) / 9.0f;
    const int   vRow   = 9 - row;
    const int   vCol   = col - 1;
    const float x = vCol * cell + (col == 9 ? gutter : 0.0f);
    const float y = vRow * cell + (row == 9 ? 0.0f : gutter);
    return { x, y, cell, cell };
}

std::pair<int, int> VirtualLaunchpad::cellAt (juce::Point<float> pt) const
{
    for (int row = 1; row <= 9; ++row)
        for (int col = 1; col <= 9; ++col)
            if (cellBounds (row, col).contains (pt))
                return { row, col };
    return { -1, -1 };
}

void VirtualLaunchpad::paint (juce::Graphics& g)
{
    const float side = static_cast<float> (juce::jmin (getWidth(), getHeight()));
    g.setColour (juce::Colour (0xff111111));
    g.fillRoundedRectangle (0.0f, 0.0f, side, side, 8.0f);

    for (int row = 1; row <= 9; ++row)
        for (int col = 1; col <= 9; ++col)
        {
            const auto view = describeLaunchpad (engine, row, col);
            const auto r    = cellBounds (row, col).reduced (3.0f);
            const bool edge = row == 9 || col == 9;

            g.setColour (view.colour.getBrightness() < 0.02f ? juce::Colour (0xff1c1c1c) : view.colour);
            if (edge) g.fillEllipse (r.reduced (r.getWidth() * 0.12f));
            else      g.fillRoundedRectangle (r, 4.0f);

            if (view.lit && ! edge)
            {
                g.setColour (juce::Colours::white.withAlpha (0.85f));
                g.drawRoundedRectangle (r, 4.0f, 1.5f);
            }

            if (view.label.isNotEmpty())
            {
                const bool brightBg = view.colour.getPerceivedBrightness() > 0.55f;
                g.setColour (brightBg ? juce::Colours::black : juce::Colours::white.withAlpha (0.85f));
                g.setFont (juce::FontOptions (edge ? juce::jlimit (7.0f, 10.0f, r.getWidth() * 0.14f)
                                                   : juce::jlimit (8.0f, 12.0f, r.getWidth() * 0.17f)));
                g.drawFittedText (view.label, r.reduced (edge ? 4.0f : 2.0f).toNearestInt(),
                                  juce::Justification::centred, 3, 0.8f);
            }
        }
}

void VirtualLaunchpad::mouseDown (const juce::MouseEvent& e)
{
    const auto [row, col] = cellAt (e.position);
    if (row < 0)
        return;

    mouseShift = e.mods.isShiftDown();
    if (mouseShift)
        engine.topButton (TopButton::Shift, true);

    if (col == 9 && row <= 8)
        engine.setViewPage (8 - row);
    else if (row == 9 && col <= 8)
    {
        pressedTop = col - 1;
        engine.topButton (static_cast<TopButton> (pressedTop), true);
    }
    else if (row <= 8 && col <= 8)
        engine.padPressed (row, col);
}

void VirtualLaunchpad::mouseUp (const juce::MouseEvent&)
{
    if (pressedTop >= 0)
        engine.topButton (static_cast<TopButton> (pressedTop), false);
    pressedTop = -1;
    if (mouseShift)
        engine.topButton (TopButton::Shift, false);
    mouseShift = false;
}

// ============================================================================
// XlPanel
// ============================================================================
XlPanel::XlPanel (LiveEngine& e, juce::LookAndFeel& knobLnf) : engine (e)
{
    for (int p = 0; p < kNumParams; ++p)
    {
        auto& s = controls[static_cast<size_t> (p)];
        const bool fader = p >= kFaderParam0;
        s.setSliderStyle (fader ? juce::Slider::LinearVertical : juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        s.setRange (0.0, 1.0, 0.0);
        s.setValue (engine.getParam (p), juce::dontSendNotification);
        s.setColour (juce::Slider::rotarySliderFillColourId, kRowAccent[p / kNumStrips]);
        s.setColour (juce::Slider::trackColourId, kRowAccent[3]);
        s.setColour (juce::Slider::backgroundColourId, juce::Colour (0xff3a3a3a));
        s.setColour (juce::Slider::thumbColourId, juce::Colours::white);
        if (! fader) s.setLookAndFeel (&knobLnf);
        s.setEnabled (kParamDefs[static_cast<size_t> (p)].used);
        s.setDoubleClickReturnValue (true, kParamDefs[static_cast<size_t> (p)].deflt);
        s.onValueChange = [this, p] { engine.setParam (p, static_cast<float> (controls[static_cast<size_t> (p)].getValue())); };
        addAndMakeVisible (s);

        auto& n = names[static_cast<size_t> (p)];
        n.setText (kParamDefs[static_cast<size_t> (p)].used ? kParamDefs[static_cast<size_t> (p)].name : "", juce::dontSendNotification);
        n.setJustificationType (juce::Justification::centred);
        n.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        n.setColour (juce::Label::textColourId, kText);
        addAndMakeVisible (n);

        auto& v = values[static_cast<size_t> (p)];
        v.setJustificationType (juce::Justification::centred);
        v.setFont (juce::FontOptions (10.0f));
        v.setColour (juce::Label::textColourId, kRowAccent[p / kNumStrips]);
        addAndMakeVisible (v);
    }

    for (int b = 0; b < kNumButtons; ++b)
    {
        buttons.push_back (std::make_unique<XlButton> (engine, b));
        addAndMakeVisible (*buttons.back());
    }
    sync();
}

XlPanel::~XlPanel()
{
    for (auto& s : controls)
        s.setLookAndFeel (nullptr);
}

void XlPanel::XlButton::paint (juce::Graphics& g)
{
    const auto view = describeXlButton (engine, index);
    const auto r    = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (view.colour);
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (view.colour.getPerceivedBrightness() > 0.5f ? juce::Colours::black : kText);
    g.setFont (juce::FontOptions (11.0f, view.lit ? juce::Font::bold : juce::Font::plain));
    g.drawFittedText (view.label, r.toNearestInt().reduced (2), juce::Justification::centred, 2);
}

void XlPanel::paint (juce::Graphics& g)
{
    g.setColour (juce::Colour (0xff161616));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 8.0f);

    g.setColour (kDimText);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ("LAUNCH CONTROL XL  ·  factory template 1", getLocalBounds().reduced (12, 6).removeFromTop (16),
                juce::Justification::centredLeft);

    // Strip separators + numbers.
    const int stripW = stripsArea.getWidth() / kNumStrips;
    for (int i = 0; i < kNumStrips; ++i)
    {
        const int x = stripsArea.getX() + i * stripW;
        g.setColour (juce::Colour (0xff222222));
        if (i > 0) g.fillRect (x, stripsArea.getY(), 1, stripsArea.getHeight());
        g.setColour (kDimText);
        g.setFont (juce::FontOptions (11.0f));
        g.drawText (juce::String (i + 1), x, stripsArea.getBottom() - 14, stripW, 14, juce::Justification::centred);
    }
}

void XlPanel::resized()
{
    auto area = getLocalBounds().reduced (10);
    area.removeFromTop (20);
    stripsArea = area;

    const int stripW  = area.getWidth() / kNumStrips;
    const int knobRow = juce::jmin (88, (area.getHeight() - 14) * 22 / 100);
    const int btnH    = 26;
    const int faderH  = area.getHeight() - 3 * knobRow - 2 * (btnH + 4) - 14;

    for (int i = 0; i < kNumStrips; ++i)
    {
        auto strip = juce::Rectangle<int> (area.getX() + i * stripW, area.getY(), stripW, area.getHeight()).reduced (6, 0);

        for (int row = 0; row < kNumKnobRows; ++row)
        {
            auto cell = strip.removeFromTop (knobRow);
            const int p = row * kNumStrips + i;
            names [static_cast<size_t> (p)].setBounds (cell.removeFromTop (14));
            values[static_cast<size_t> (p)].setBounds (cell.removeFromBottom (13));
            controls[static_cast<size_t> (p)].setBounds (cell);
        }

        auto faderCell = strip.removeFromTop (faderH);
        const int fp = kFaderParam0 + i;
        names [static_cast<size_t> (fp)].setBounds (faderCell.removeFromTop (14));
        values[static_cast<size_t> (fp)].setBounds (faderCell.removeFromBottom (13));
        controls[static_cast<size_t> (fp)].setBounds (faderCell.withSizeKeepingCentre (36, faderCell.getHeight()));

        strip.removeFromTop (4);
        buttons[static_cast<size_t> (i)]->setBounds (strip.removeFromTop (btnH));
        strip.removeFromTop (4);
        buttons[static_cast<size_t> (kNumStrips + i)]->setBounds (strip.removeFromTop (btnH));
    }
}

void XlPanel::sync()
{
    const int bars = engine.getRig().cols;
    for (int p = 0; p < kNumParams; ++p)
    {
        auto& s = controls[static_cast<size_t> (p)];
        const float v = engine.getParam (p);
        if (! s.isMouseButtonDown() && std::abs (s.getValue() - v) > 1.0e-4)
            s.setValue (v, juce::dontSendNotification);

        // Faders beyond the grid's bar count do nothing — grey them out.
        if (p >= kFaderParam0)
        {
            const bool live = p - kFaderParam0 < bars;
            s.setEnabled (live);
            names[static_cast<size_t> (p)].setColour (juce::Label::textColourId, live ? kText : kDimText.darker());
        }
        values[static_cast<size_t> (p)].setText (formatParam (p, v), juce::dontSendNotification);
    }
    for (auto& b : buttons)
        b->repaint();
}

// ============================================================================
// MainComponent
// ============================================================================
MainComponent::MainComponent()
{
    setSize (1460, 880);

    addAndMakeVisible (visualizer);
    addAndMakeVisible (virtualLaunchpad);
    addAndMakeVisible (xlPanel);

    // ── DMX ──
    addAndMakeVisible (dmxButton);
    dmxButton.onClick = [this]
    {
        if (dmx.isRunning()) dmx.disconnect();
        else                 dmx.connect();
    };
    addAndMakeVisible (dmxStatus);
    dmxStatus.setFont (juce::FontOptions (11.0f));
    dmxStatus.setJustificationType (juce::Justification::topLeft);
    dmxStatus.setColour (juce::Label::textColourId, kDimText);

    // ── MIDI devices ──
    for (auto* l : { &lpLabel, &xlLabel, &gridLabel })
    {
        l->setFont (juce::FontOptions (11.0f));
        l->setColour (juce::Label::textColourId, kDimText);
        addAndMakeVisible (*l);
    }
    for (auto* c : { &lpIn, &lpOut, &xlIn, &xlOut })
    {
        addAndMakeVisible (*c);
        c->onChange = [this] { openSurfacesFromCombos(); };
    }
    addAndMakeVisible (rescanButton);
    rescanButton.onClick = [this] { rescanDevices(); };

    // ── grid ──
    for (int c = 1; c <= hitnotedmx::kMaxBars; ++c) gridCols.addItem (juce::String (c), c);
    for (int r = 1; r <= hitnotedmx::kMaxRows; ++r) gridRows.addItem (juce::String (r), r);
    addAndMakeVisible (gridCols);
    addAndMakeVisible (gridRows);
    addAndMakeVisible (gridSet);
    gridSet.onClick = [this]
    {
        const auto rig = engine.setGridShape (gridCols.getSelectedId(), gridRows.getSelectedId());
        gridCols.setSelectedId (rig.cols, juce::dontSendNotification);
        gridRows.setSelectedId (rig.rows, juce::dontSendNotification);
    };

    // ── tempo + log ──
    addAndMakeVisible (tempoLabel);
    tempoLabel.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    tempoLabel.setColour (juce::Label::textColourId, kText);
    tempoLabel.setJustificationType (juce::Justification::centredLeft);

    addAndMakeVisible (logView);
    logView.setMultiLine (true);
    logView.setReadOnly (true);
    logView.setScrollbarsShown (true);
    logView.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 10.0f, juce::Font::plain)));
    logView.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff161616));
    logView.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);

    loadState();
    const auto rig = engine.getRig();
    gridCols.setSelectedId (rig.cols, juce::dontSendNotification);
    gridRows.setSelectedId (rig.rows, juce::dontSendNotification);

    rescanDevices();
    engine.start();
    startTimerHz (30);
}

MainComponent::~MainComponent()
{
    stopTimer();
    saveState();
    launchpad.close();
    xlSurface.close();
    engine.stop();
    dmx.disconnect();
}

void MainComponent::rescanDevices()
{
    const auto fill = [] (juce::ComboBox& box, const juce::Array<juce::MidiDeviceInfo>& devs, const juce::String& pick)
    {
        const auto current = box.getText();
        box.clear (juce::dontSendNotification);
        box.addItem ("(none)", 1);
        int id = 2;
        for (const auto& d : devs)
            box.addItem (d.name, id++);
        const auto want = current.isNotEmpty() && current != "(none)" ? current : pick;
        box.setSelectedId (1, juce::dontSendNotification);
        for (int i = 0; i < box.getNumItems(); ++i)
            if (box.getItemText (i) == want)
                box.setSelectedItemIndex (i, juce::dontSendNotification);
    };

    const auto ins  = juce::MidiInput::getAvailableDevices();
    const auto outs = juce::MidiOutput::getAvailableDevices();
    fill (lpIn,  ins,  LaunchpadSurface::findInput());
    fill (lpOut, outs, LaunchpadSurface::findOutput());
    fill (xlIn,  ins,  LaunchControlXlSurface::findInput());
    fill (xlOut, outs, LaunchControlXlSurface::findOutput());
    openSurfacesFromCombos();
}

void MainComponent::openSurfacesFromCombos()
{
    const auto name = [] (const juce::ComboBox& b) { return b.getSelectedId() > 1 ? b.getText() : juce::String(); };
    launchpad.open (name (lpIn), name (lpOut));
    xlSurface.open (name (xlIn), name (xlOut));
    lastSerial = lastPadSerial = 0xffffffff;   // force an LED refresh
}

void MainComponent::timerCallback()
{
    // Hardware LEDs + virtual surfaces follow the engine state.
    const auto serial    = engine.getStateSerial();
    const auto padSerial = engine.getPadSerial();
    const double ph      = engine.getGlobalBeats() - std::floor (engine.getGlobalBeats());
    const bool blink     = ph < 0.15;
    if (padSerial != lastPadSerial)
    {
        launchpad.refreshLeds();
        virtualLaunchpad.repaint();
    }
    lastPadSerial = padSerial;
    if (serial != lastSerial || blink != lastTapBlink)
    {
        xlSurface.refreshLeds();
        xlPanel.sync();
    }
    lastSerial   = serial;
    lastTapBlink = blink;

    visualizer.setGrid (engine.getRig());
    visualizer.setStrobe (engine.getStrobeHz());
    visualizer.repaintIfChanged();

    // Tempo readout: BPM, applied relative speed, beat-in-bar dots.
    const int beatInBar = static_cast<int> (std::floor (engine.getGlobalBeats())) % 4;
    juce::String dots;
    for (int i = 0; i < 4; ++i) dots << (i == beatInBar ? juce::String::fromUTF8 ("\xe2\x97\x8f ") : juce::String::fromUTF8 ("\xe2\x97\x8b "));
    const double mult = engine.getAppliedRelSpeed();
    tempoLabel.setText (juce::String (engine.getBpm(), 1) + " BPM   "
                        + (mult >= 1.0 ? juce::String (juce::roundToInt (mult)) : "1/" + juce::String (juce::roundToInt (1.0 / mult)))
                        + "x   " + dots, juce::dontSendNotification);

    dmxButton.setButtonText (dmx.isRunning() ? "Disconnect DMX" : "Connect DMX");
    dmxStatus.setText (dmx.getStatusText(), juce::dontSendNotification);

    juce::String logText;
    if (activity.takeIfChanged (logText))
    {
        logView.setText (logText, false);
        logView.moveCaretToEnd();
    }

    // Persist a few seconds after the last change (and on quit).
    if (serial != savedSerial)
    {
        if (saveCountdown <= 0) saveCountdown = 90;   // 3 s at 30 Hz
        if (--saveCountdown == 0) { saveState(); savedSerial = serial; }
    }
}

juce::File MainComponent::stateFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("Application Support/HitLaunchDmx/state.xml");
}

void MainComponent::loadState()
{
    if (auto xml = juce::XmlDocument::parse (stateFile()))
        engine.fromXml (*xml);
    savedSerial = engine.getStateSerial();
}

void MainComponent::saveState()
{
    const auto f = stateFile();
    f.getParentDirectory().createDirectory();
    if (auto xml = engine.toXml())
        xml->writeTo (f);
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (kBg);

    // Left panel background + title strip above the visualiser.
    g.setColour (kPanel);
    g.fillRoundedRectangle (8.0f, 8.0f, 230.0f, static_cast<float> (xlPanel.getY() - 16), 6.0f);

    g.setColour (kPink);
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText ("Flamingo Hitmix at Night  ·  HitLaunchDmx",
                visualizer.getX(), 8, visualizer.getWidth(), 28, juce::Justification::centred);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced (8);

    // Bottom: the XL panel spans the width.
    xlPanel.setBounds (area.removeFromBottom (juce::jmax (330, area.getHeight() * 40 / 100)));
    area.removeFromBottom (8);

    // Left panel.
    auto left = area.removeFromLeft (230).reduced (10);
    area.removeFromLeft (10);
    dmxButton.setBounds (left.removeFromTop (28));
    left.removeFromTop (4);
    dmxStatus.setBounds (left.removeFromTop (44));
    left.removeFromTop (6);
    lpLabel.setBounds (left.removeFromTop (16));
    lpIn.setBounds (left.removeFromTop (24));  left.removeFromTop (3);
    lpOut.setBounds (left.removeFromTop (24)); left.removeFromTop (6);
    xlLabel.setBounds (left.removeFromTop (16));
    xlIn.setBounds (left.removeFromTop (24));  left.removeFromTop (3);
    xlOut.setBounds (left.removeFromTop (24)); left.removeFromTop (4);
    rescanButton.setBounds (left.removeFromTop (26));
    left.removeFromTop (8);
    gridLabel.setBounds (left.removeFromTop (16));
    {
        auto row = left.removeFromTop (26);
        gridCols.setBounds (row.removeFromLeft (60));
        row.removeFromLeft (4);
        gridRows.setBounds (row.removeFromLeft (60));
        row.removeFromLeft (4);
        gridSet.setBounds (row);
    }
    left.removeFromTop (8);
    tempoLabel.setBounds (left.removeFromTop (30));
    left.removeFromTop (6);
    logView.setBounds (left);

    // Right: the virtual Launchpad, square, full height of the top section.
    const int lpSide = juce::jmin (area.getHeight(), area.getWidth() / 2);
    virtualLaunchpad.setBounds (area.removeFromRight (lpSide).withSizeKeepingCentre (lpSide, lpSide));
    area.removeFromRight (10);

    // Centre: title strip + visualiser.
    area.removeFromTop (32);
    visualizer.setBounds (area);
}

}  // namespace hitlaunchdmx
