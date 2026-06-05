#include "MainComponent.h"
#include "Scene.h"

namespace hitlaunchdmx
{

MainComponent::MainComponent()
    : engine(dmx, liveFrame, midiLog, 2.0f),
      visualizer(liveFrame, 0)
{
    setSize(960, 640);

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
    midiLogEditor.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 10.0f, juce::Font::plain)));

    // ── centre visualiser ─────────────────────────────────────────────────
    addAndMakeVisible(visualizer);

    // ── right panel ───────────────────────────────────────────────────────
    addAndMakeVisible(sceneNameLabel);
    addAndMakeVisible(sceneValueLabel);
    sceneValueLabel.setFont(juce::FontOptions(13.0f));
    sceneValueLabel.setJustificationType(juce::Justification::topLeft);

    addAndMakeVisible(xfadeLabel);
    addAndMakeVisible(xfadeSlider);
    xfadeSlider.setRange(0.1, 5.0, 0.1);
    xfadeSlider.setValue(2.0);
    xfadeSlider.setTextValueSuffix(" s");
    xfadeSlider.onValueChange = [this] {
        engine.setCrossfadeSec(static_cast<float>(xfadeSlider.getValue()));
    };

    // Populate device menus and start engine
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
    {
        if (midiInCombo.getItemText(i).containsIgnoreCase("Launchpad"))
        {
            midiInCombo.setSelectedItemIndex(i, juce::dontSendNotification);
            engine.openMidiInput(midiInCombo.getItemText(i));
        }
    }
    for (int i = 0; i < midiOutCombo.getNumItems(); ++i)
    {
        if (midiOutCombo.getItemText(i).containsIgnoreCase("Launchpad"))
        {
            midiOutCombo.setSelectedItemIndex(i, juce::dontSendNotification);
            engine.openMidiOutput(midiOutCombo.getItemText(i));
        }
    }

    midiInCombo.onChange = [this] {
        const int idx = midiInCombo.getSelectedItemIndex();
        if (idx >= 0)
            engine.openMidiInput(midiInCombo.getItemText(idx));
    };
    midiOutCombo.onChange = [this] {
        const int idx = midiOutCombo.getSelectedItemIndex();
        if (idx >= 0)
            engine.openMidiOutput(midiOutCombo.getItemText(idx));
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
    // Update scene label
    const int activeIdx = engine.getActiveScene();
    visualizer.setActiveScene(activeIdx);
    if (activeIdx >= 0 && activeIdx < 64)
        sceneValueLabel.setText(juce::String(kScenes[static_cast<size_t>(activeIdx)].name),
                                juce::dontSendNotification);

    // Drain MIDI log
    juce::String newLines;
    while (auto entry = midiLog.pop())
    {
        const auto& e = *entry;
        const juce::String kind = e.kind == MidiLogEntry::Kind::NoteOn ? "ON " : "off";
        newLines += "[" + kind + "] ch" + juce::String(e.channel)
                 + " n=" + juce::String(e.pitch)
                 + " v=" + juce::String(e.velocity) + "\n";
    }
    if (newLines.isNotEmpty())
    {
        const juce::String existing = midiLogEditor.getText();
        const juce::StringArray lines = juce::StringArray::fromLines(existing + newLines);
        // Keep last 20 lines
        const int start = juce::jmax(0, lines.size() - 20);
        juce::String trimmed;
        for (int i = start; i < lines.size(); ++i)
            trimmed += lines[i] + "\n";
        midiLogEditor.setText(trimmed, false);
        midiLogEditor.moveCaretToEnd();
    }

    // Update DMX status occasionally
    dmxStatusLabel.setText(dmx.getStatusText(), juce::dontSendNotification);

    visualizer.repaintIfChanged();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));
}

void MainComponent::resized()
{
    auto area   = getLocalBounds().reduced(8);
    const int leftW  = 180;
    const int rightW = 180;

    auto leftPanel   = area.removeFromLeft(leftW);
    auto rightPanel  = area.removeFromRight(rightW);
    auto centrePanel = area;

    // Left panel
    midiInLabel.setBounds(leftPanel.removeFromTop(16));
    midiInCombo.setBounds(leftPanel.removeFromTop(24));
    leftPanel.removeFromTop(4);
    midiOutLabel.setBounds(leftPanel.removeFromTop(16));
    midiOutCombo.setBounds(leftPanel.removeFromTop(24));
    leftPanel.removeFromTop(8);
    connectBtn.setBounds(leftPanel.removeFromTop(26));
    leftPanel.removeFromTop(4);
    disconnectBtn.setBounds(leftPanel.removeFromTop(26));
    leftPanel.removeFromTop(4);
    blackoutBtn.setBounds(leftPanel.removeFromTop(26));
    leftPanel.removeFromTop(4);
    lpModeBtn.setBounds(leftPanel.removeFromTop(26));
    leftPanel.removeFromTop(8);
    dmxStatusLabel.setBounds(leftPanel.removeFromTop(52));
    leftPanel.removeFromTop(4);
    midiLogEditor.setBounds(leftPanel);

    // Centre visualiser
    visualizer.setBounds(centrePanel.reduced(8));

    // Right panel
    sceneNameLabel.setBounds(rightPanel.removeFromTop(16));
    sceneValueLabel.setBounds(rightPanel.removeFromTop(48));
    rightPanel.removeFromTop(16);
    xfadeLabel.setBounds(rightPanel.removeFromTop(16));
    xfadeSlider.setBounds(rightPanel.removeFromTop(40));
}

}  // namespace hitlaunchdmx
