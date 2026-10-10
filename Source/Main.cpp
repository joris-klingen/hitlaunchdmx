#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "MainComponent.h"

namespace hitlaunchdmx
{

class HitLaunchDmxApp : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override    { return "HitLaunchDmx"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override          { return false; }

    void initialise(const juce::String&) override
    {
        mainWindow = std::make_unique<MainWindow>(getApplicationName());
    }

    void shutdown() override { mainWindow.reset(); }

    void systemRequestedQuit() override { quit(); }

    struct MainWindow : public juce::DocumentWindow
    {
        explicit MainWindow(const juce::String& name)
            : juce::DocumentWindow(name,
                                   juce::Colours::darkgrey,
                                   juce::DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new MainComponent(), true);
            setResizable(true, true);
            centreWithSize(getWidth(), getHeight());
            setVisible(true);
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

}  // namespace hitlaunchdmx

START_JUCE_APPLICATION(hitlaunchdmx::HitLaunchDmxApp)
