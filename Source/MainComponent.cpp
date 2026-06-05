#include "MainComponent.h"
#include "Scene.h"

#include <cmath>

namespace hitlaunchdmx
{

// ============================================================================
// VirtualLaunchpad
// ============================================================================

VirtualLaunchpad::VirtualLaunchpad(SceneEngine& eng) : engine(eng)
{
    setOpaque(true);
}

juce::Rectangle<float> VirtualLaunchpad::cellBoundsF(int vRow, int vCol) const
{
    const float padW = static_cast<float>(getWidth())  / 8.0f;
    const float padH = static_cast<float>(getHeight()) / 8.0f;
    return { vCol * padW + 1.5f, vRow * padH + 1.5f, padW - 3.0f, padH - 3.0f };
}

int VirtualLaunchpad::cellForPoint(juce::Point<int> pt) const
{
    const float padW = static_cast<float>(getWidth())  / 8.0f;
    const float padH = static_cast<float>(getHeight()) / 8.0f;
    const int vCol = juce::jlimit(0, 7, static_cast<int>(pt.x / padW));
    const int vRow = juce::jlimit(0, 7, static_cast<int>(pt.y / padH));
    return sceneForCell(vRow, vCol);
}

void VirtualLaunchpad::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff111111));

    const float padH = static_cast<float>(getHeight()) / 8.0f;

    for (int vRow = 0; vRow < 8; ++vRow)
    {
        for (int vCol = 0; vCol < 8; ++vCol)
        {
            const int sceneIdx = sceneForCell(vRow, vCol);
            const auto& sc     = kScenes[static_cast<size_t>(sceneIdx)];
            const auto  bounds = cellBoundsF(vRow, vCol);
            const bool  active  = (sceneIdx == activeScene);
            const bool  hovered = (sceneIdx == hoveredScene);

            // Category background tint (behind pad colour)
            // vRow 0-1 = wild, 2-3 = medium, 4-7 = breathes
            float catR = 0.07f, catG = 0.07f, catB = 0.07f;
            if (vRow <= 1)      { catR = 0.10f; catG = 0.04f; catB = 0.04f; }  // wild: dark red
            else if (vRow <= 3) { catR = 0.04f; catG = 0.04f; catB = 0.12f; }  // medium: dark blue

            // Pad colour from scene's dominant color, at reduced brightness
            float mul = active ? 0.85f : (hovered ? 0.55f : 0.30f);
            // Ensure pad is never too dark to read (minimum visible)
            const float maxC = juce::jmax(sc.color1.r, sc.color1.g, sc.color1.b);
            if (maxC < 0.15f) mul = juce::jmax(mul, 0.6f);  // boost very-dark colours

            const float pr = juce::jlimit(0.0f, 1.0f, sc.color1.r * mul + catR);
            const float pg = juce::jlimit(0.0f, 1.0f, sc.color1.g * mul + catG);
            const float pb = juce::jlimit(0.0f, 1.0f, sc.color1.b * mul + catB);
            g.setColour(juce::Colour::fromFloatRGBA(pr, pg, pb, 1.0f));
            g.fillRoundedRectangle(bounds, 3.0f);

            // Active border
            if (active)
            {
                g.setColour(juce::Colours::white.withAlpha(0.9f));
                g.drawRoundedRectangle(bounds, 3.0f, 2.0f);
            }
            else if (hovered)
            {
                g.setColour(juce::Colours::white.withAlpha(0.35f));
                g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
            }

            // Scene name text
            const float textBrightness = active ? 1.0f : (hovered ? 0.95f : 0.80f);
            g.setColour(juce::Colours::white.withAlpha(textBrightness));
            g.setFont(juce::FontOptions(8.5f));
            g.drawFittedText(sc.name, bounds.toNearestInt().reduced(2),
                             juce::Justification::centred, 3, 0.85f);
        }
    }

    // Category divider lines
    g.setColour(juce::Colour(0xff333333));
    for (int divRow : { 2, 4 })  // between wild/medium and medium/breathe
    {
        const float y = divRow * padH;
        g.drawLine(0.0f, y, static_cast<float>(getWidth()), y, 1.5f);
    }

}

void VirtualLaunchpad::mouseDown(const juce::MouseEvent& e)
{
    const int idx = cellForPoint(e.getPosition());
    if (idx >= 0 && idx < 64)
    {
        activeScene = idx;
        engine.setActiveScene(idx);
        repaint();
    }
}

void VirtualLaunchpad::mouseMove(const juce::MouseEvent& e)
{
    const int idx = cellForPoint(e.getPosition());
    if (idx != hoveredScene)
    {
        hoveredScene = idx;
        repaint();
    }
}

void VirtualLaunchpad::mouseExit(const juce::MouseEvent&)
{
    hoveredScene = -1;
    repaint();
}

// ============================================================================
// MainComponent
// ============================================================================

MainComponent::MainComponent()
    : engine(dmx, liveFrame, midiLog, 2.0f),
      visualizer(liveFrame, 0),
      virtualLaunchpad(engine)
{
    setSize(1160, 700);

    // ── left panel ────────────────────────────────────────────────────────
    addAndMakeVisible(midiInLabel);
    addAndMakeVisible(midiInCombo);
    addAndMakeVisible(midiOutLabel);
    addAndMakeVisible(midiOutCombo);

    addAndMakeVisible(connectBtn);
    connectBtn.onClick = [this] { connectDmx(); };

    addAndMakeVisible(disconnectBtn);
    disconnectBtn.onClick = [this] { disconnectDmx(); };

    addAndMakeVisible(blackoutBtn);
    blackoutBtn.setClickingTogglesState(true);
    blackoutBtn.onClick = [this] {
        blackoutActive = blackoutBtn.getToggleState();
        dmx.setBlackout(blackoutActive);
    };

    addAndMakeVisible(lpModeBtn);
    lpModeBtn.onClick = [this] {
        engine.sendLaunchpadProgrammerMode();
        engine.refreshLaunchpadLEDs();
    };

    addAndMakeVisible(dmxStatusLabel);
    dmxStatusLabel.setJustificationType(juce::Justification::topLeft);
    dmxStatusLabel.setFont(juce::FontOptions(11.0f));

    addAndMakeVisible(midiLogEditor);
    midiLogEditor.setMultiLine(true);
    midiLogEditor.setReadOnly(true);
    midiLogEditor.setScrollbarsShown(true);
    midiLogEditor.setFont(juce::Font(juce::FontOptions(
        juce::Font::getDefaultMonospacedFontName(), 10.0f, juce::Font::plain)));

    // ── centre visualiser ─────────────────────────────────────────────────
    addAndMakeVisible(visualizer);

    // ── right: virtual launchpad + crossfade ─────────────────────────────
    addAndMakeVisible(virtualLaunchpad);

    addAndMakeVisible(xfadeLabel);
    addAndMakeVisible(xfadeSlider);
    xfadeSlider.setRange(0.0, 5.0, 0.1);
    xfadeSlider.setValue(2.0);
    xfadeSlider.setTextValueSuffix(" s");
    xfadeSlider.onValueChange = [this] {
        engine.setCrossfadeSec(static_cast<float>(xfadeSlider.getValue()));
    };

    buildMidiDeviceMenus();
    engine.start();
    startTimerHz(15);
}

MainComponent::~MainComponent()
{
    stopTimer();
    engine.stop();
    dmx.disconnect();
}

void MainComponent::buildMidiDeviceMenus()
{
    midiInCombo.clear(juce::dontSendNotification);
    midiOutCombo.clear(juce::dontSendNotification);

    int id = 1;
    for (const auto& name : engine.getMidiInputNames())
        midiInCombo.addItem(name, id++);

    id = 1;
    for (const auto& name : engine.getMidiOutputNames())
        midiOutCombo.addItem(name, id++);

    // Auto-select Launchpad if present
    for (int i = 0; i < midiInCombo.getNumItems(); ++i)
        if (midiInCombo.getItemText(i).containsIgnoreCase("Launchpad"))
        {
            midiInCombo.setSelectedItemIndex(i, juce::dontSendNotification);
            engine.openMidiInput(midiInCombo.getItemText(i));
        }

    for (int i = 0; i < midiOutCombo.getNumItems(); ++i)
        if (midiOutCombo.getItemText(i).containsIgnoreCase("Launchpad"))
        {
            midiOutCombo.setSelectedItemIndex(i, juce::dontSendNotification);
            engine.openMidiOutput(midiOutCombo.getItemText(i));
        }

    midiInCombo.onChange = [this] {
        const int idx = midiInCombo.getSelectedItemIndex();
        if (idx >= 0) engine.openMidiInput(midiInCombo.getItemText(idx));
    };
    midiOutCombo.onChange = [this] {
        const int idx = midiOutCombo.getSelectedItemIndex();
        if (idx >= 0) engine.openMidiOutput(midiOutCombo.getItemText(idx));
    };
}

void MainComponent::connectDmx()
{
    dmx.connect();
    dmxStatusLabel.setText(dmx.getStatusText(), juce::dontSendNotification);
}

void MainComponent::disconnectDmx()
{
    dmx.disconnect();
    dmxStatusLabel.setText(dmx.getStatusText(), juce::dontSendNotification);
}

void MainComponent::timerCallback()
{
    const int activeIdx = engine.getActiveScene();

    // Keep virtual launchpad in sync with MIDI-triggered changes
    virtualLaunchpad.setActiveScene(activeIdx);

    // Drain MIDI log (capped at 20 lines)
    juce::String newLines;
    while (auto entry = midiLog.pop())
    {
        const auto& e = *entry;
        newLines += juce::String(e.kind == MidiLogEntry::Kind::NoteOn ? "[ON ] " : "[off] ")
                 + "ch" + juce::String(e.channel)
                 + " n="  + juce::String(e.pitch)
                 + " v="  + juce::String(e.velocity) + "\n";
    }
    if (newLines.isNotEmpty())
    {
        const juce::StringArray lines =
            juce::StringArray::fromLines(midiLogEditor.getText() + newLines);
        const int start = juce::jmax(0, lines.size() - 20);
        juce::String trimmed;
        for (int i = start; i < lines.size(); ++i)
            trimmed += lines[i] + "\n";
        midiLogEditor.setText(trimmed, false);
        midiLogEditor.moveCaretToEnd();
    }

    dmxStatusLabel.setText(dmx.getStatusText(), juce::dontSendNotification);
    visualizer.repaintIfChanged();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(8);
    const int leftW  = 180;
    const int rightW = 490;

    auto leftPanel  = area.removeFromLeft(leftW);
    area.removeFromLeft(6);
    auto rightPanel = area.removeFromRight(rightW);
    area.removeFromRight(6);
    auto centrePanel = area;

    // ── left ──────────────────────────────────────────────────────────────
    midiInLabel  .setBounds(leftPanel.removeFromTop(16));
    midiInCombo  .setBounds(leftPanel.removeFromTop(24));
    leftPanel.removeFromTop(4);
    midiOutLabel .setBounds(leftPanel.removeFromTop(16));
    midiOutCombo .setBounds(leftPanel.removeFromTop(24));
    leftPanel.removeFromTop(8);
    connectBtn   .setBounds(leftPanel.removeFromTop(26));
    leftPanel.removeFromTop(4);
    disconnectBtn.setBounds(leftPanel.removeFromTop(26));
    leftPanel.removeFromTop(4);
    blackoutBtn  .setBounds(leftPanel.removeFromTop(26));
    leftPanel.removeFromTop(4);
    lpModeBtn    .setBounds(leftPanel.removeFromTop(26));
    leftPanel.removeFromTop(8);
    dmxStatusLabel.setBounds(leftPanel.removeFromTop(50));
    leftPanel.removeFromTop(4);
    midiLogEditor.setBounds(leftPanel);

    // ── centre ────────────────────────────────────────────────────────────
    visualizer.setBounds(centrePanel.reduced(4));

    // ── right: launchpad grid + crossfade ─────────────────────────────────
    // Crossfade at the bottom
    rightPanel.removeFromBottom(4);
    xfadeSlider.setBounds(rightPanel.removeFromBottom(36));
    xfadeLabel .setBounds(rightPanel.removeFromBottom(18));
    rightPanel.removeFromBottom(8);

    // Virtual launchpad fills the rest
    virtualLaunchpad.setBounds(rightPanel);
}

}  // namespace hitlaunchdmx
