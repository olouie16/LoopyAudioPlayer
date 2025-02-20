#pragma once

#include <JuceHeader.h>
#include "TimeLine.h"
#include "AudioFile.h"
#include "OwnLookAndFeel.h"
#include "mod_FileBrowserComponent.h"





class FileBrowserCompHelper 
{
public:
    FileBrowserCompHelper() : thread("fileThread"),
        audioFileFilter("*.mp3;*.wav;*.flac;*.ogg;*.wmv;*.aif;*.aiff;*.asf;*.wma;*.wm;*.bwf", "*", "AudioFiles")
    {}

    juce::TimeSliceThread thread;
    const juce::WildcardFileFilter audioFileFilter;
};

class FileBrowserComp : public FileBrowserCompHelper, public juce::mod::FileBrowserComponent
{
public:

    FileBrowserComp(OwnLookAndFeel* myLookAndFeel_): juce::mod::FileBrowserComponent(juce::FileBrowserComponent::FileChooserFlags::openMode + juce::FileBrowserComponent::FileChooserFlags::canSelectFiles,
                                                    juce::File(), &audioFileFilter, nullptr, myLookAndFeel_)
    {

    }

    void getRoots(juce::StringArray& rootNames, juce::StringArray& rootPaths) override;


    void updateMusicLibsInCombo() { resetRecentPaths(); }


    void setAdditionalPathsInCombo(std::vector<juce::File>* v) { additionalPathsInCombo = v; };
private:
    std::vector<juce::File>* additionalPathsInCombo = nullptr;

};


class ControlButton : public juce::ImageButton
{
public:
    ControlButton() {};
    ~ControlButton() {};

    void setImage(juce::Image& image);
    void resized();

    std::function< void()> onLeftClick;
    std::function< void()> onRightClick;

protected:
    void clicked(const juce::ModifierKeys& modifiers) override{
        if (modifiers.isLeftButtonDown()) {
            if (onLeftClick)
                onLeftClick();
        }
        if (modifiers.isRightButtonDown()) {
            if (onRightClick)
                onRightClick();
        }
    }

private:
    juce::Image normalImage;
    juce::Image overImage;
    juce::Image downImage;

};



class SettingsViewContentComponent :public juce::Component{
public:
    SettingsViewContentComponent() {

        setInterceptsMouseClicks(false, true);

        audioSettingsButton.setButtonText("Audio Settings");
        addAndMakeVisible(audioSettingsButton);

        musicLibsLabel.setText("Music Librarys", juce::NotificationType::dontSendNotification);
        musicLibsLabel.setJustificationType(juce::Justification::centredLeft);
        musicLibsLabel.setFont(juce::Font(14));
        musicLibsLabel.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(musicLibsLabel);
       

        addAndMakeVisible(pathsCombo);

        delButton.setButtonText("delete");
        addAndMakeVisible(delButton);

        backButton.setButtonText("back");
        addAndMakeVisible(backButton);


        crossFadeTitelLabel.setText("default CrossFade Settings (used if not changed for this file yet)", juce::NotificationType::dontSendNotification);
        crossFadeTitelLabel.setJustificationType(juce::Justification::centredLeft);
        crossFadeTitelLabel.setFont(juce::Font(14));
        crossFadeTitelLabel.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(crossFadeTitelLabel);
        

        defaultCrossFadeToggle.setButtonText("active");
        addAndMakeVisible(defaultCrossFadeToggle);

        defaultCrossFadeLabel.setEditable(true);
        defaultCrossFadeLabel.setText("0", juce::NotificationType::dontSendNotification);
        defaultCrossFadeLabel.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(defaultCrossFadeLabel);

        crossFadeUnitLabel.setText("secs", juce::NotificationType::dontSendNotification);
        crossFadeUnitLabel.setJustificationType(juce::Justification::centredLeft);
        crossFadeUnitLabel.setFont(juce::Font(12));
        crossFadeUnitLabel.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(crossFadeUnitLabel);
    }
    ~SettingsViewContentComponent() {};

    juce::FlexBox fb;
    juce::FlexBox fbMusicLibs;
    juce::FlexBox fbCrossFade;

    juce::TextButton audioSettingsButton;
    juce::Label musicLibsLabel;
    juce::ComboBox pathsCombo;
    juce::TextButton delButton;
    juce::TextButton backButton;

    juce::Label crossFadeTitelLabel;
    juce::ToggleButton defaultCrossFadeToggle;
    juce::Label defaultCrossFadeLabel;
    juce::Label crossFadeUnitLabel;


    void resized() {

        initFlexBoxes();
        fb.performLayout(getLocalBounds());
    }
private:
    void initFlexBoxes();
};

class SettingsViewWindow :public juce::DocumentWindow {

public:
    SettingsViewWindow(): juce::DocumentWindow("Settings", juce::Colours::white, juce::DocumentWindow::TitleBarButtons::closeButton, true)
    {

        settingsViewContentComponent.delButton.onClick = [this] {onDelButtonClicked(); };
        settingsViewContentComponent.backButton.onClick = [this] {closeButtonPressed(); };
        settingsViewContentComponent.audioSettingsButton.onClick = [this] {onAudioSettingsButtonClicked(); };
        settingsViewContentComponent.defaultCrossFadeToggle.onStateChange = [this] {onDefaultCrossFadeToggleChange(); };
        defaultCrossFadeLabel = &settingsViewContentComponent.defaultCrossFadeLabel;


        setContentComponent(&settingsViewContentComponent);
        setBackgroundColour(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
        setSize(550, 350);
        setResizable(false, false);
        setDraggable(true);


    };
    ~SettingsViewWindow() {};

    void closeButtonPressed() {

        exitModalState();
        setVisible(false);

    }


    SettingsViewContentComponent settingsViewContentComponent;

    juce::Label* defaultCrossFadeLabel;

    std::function<void()> onDelButtonClicked;
    std::function<void()> onAudioSettingsButtonClicked;
    std::function<void()> onDefaultCrossFadeToggleChange;
    //std::function<void()> onDefaultCrossFadeTextEditShow;
    //std::function<void()> onDefaultCrossFadeTextEditHide;
};


//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent : public juce::AudioAppComponent,
                      public juce::FileBrowserListener,
                      public juce::ChangeListener,
                      public juce::ComponentBoundsConstrainer
{
public:
    //==============================================================================
    MainComponent() ;
    ~MainComponent() override;
    //==============================================================================
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;
    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;




private:
    //==============================================================================
    // Your private member variables go here...


    ControlButton playButton;
    ControlButton stopButton;
    ControlButton loopButton;
    ControlButton settingsButton;
    juce::Slider volSlider;
    juce::ToggleButton crossFadeCheckBox;
    juce::Label crossFadeLabel;
    class CrossFadeEditFilter : public juce::TextEditor::InputFilter {
        juce::String filterNewText(juce::TextEditor& edit, const juce::String& newInput) override {

            if (newInput.containsOnly("0123456789.")) {
                return newInput;
            }
            else {
                return "";
            }

        }
    };
    CrossFadeEditFilter crossFadeEditFilter;
    juce::Label crossFadeUnit;


    TimeLine timeLine;

    enum TransportState
    {
        Stopped,
        Starting,
        Playing,
        Pausing,
        Paused,
        Stopping
    };
    TransportState state;

    enum Loopmode
    {
        notLooping,
        loopWhole,
        loopSection,
        fakeLoopSection     //visuals like loopSection, but without real loop. this exists for the case of the playhead appearing after the looped section
    };
    Loopmode loopmode;


    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;
    juce::AudioSourceChannelInfo transitionChannelInfo;
    juce::AudioSampleBuffer transitionBuffer;
    double curSampleRate = 0;
    double curVolume=1;

    double crossFade = 0;
    bool inTransition = false;
    double crossFadeProgress = 0;
    juce::int64 otherNextReadPos;
    int curPosition=0;

    bool defaultCrossFadeActive = false;
    double defaultCrossFadeLength = 0;

    double loopStartTime;
    double loopEndTime;
    double fadeStartTime;
    double fadeEndTime;

    juce::FlexBox flexBox;
    juce::FlexBox bottomRowFb;
    void initFlexBox();

    juce::AudioDeviceManager customDeviceManager;
    juce::AudioDeviceSelectorComponent deviceSelector;
    juce::File audioDeviceSettings;

    OwnLookAndFeel myLookAndFeel;

    FileBrowserComp fileBrowser{ &myLookAndFeel };
    std::vector<juce::File> musicLibs;
    std::vector<AudioFile> allFiles;
    AudioFile* currentFile=nullptr;

    void addMusicLib(const juce::String& libToAdd);
    void removeMusicLib(const juce::String& libToRemove);
    void musicLibChanged();

    SettingsViewWindow settingsViewWindow;

    class AudioDeviceSelectorWindow : public juce::DocumentWindow
    {
    public:
        AudioDeviceSelectorWindow() : juce::DocumentWindow("Audio Device Settings",
            juce::Colours::white,
            juce::DocumentWindow::TitleBarButtons::closeButton,
            true)
        {}

        void closeButtonPressed() override
        {
            setVisible(false);
        }
    };
    AudioDeviceSelectorWindow deviceSelectorWindow;


    juce::Image playImage;
    juce::Image pauseImage;
    juce::Image stopImage;
    juce::Image noLoopImage;
    juce::Image wholeLoopImage;
    juce::Image sectionLoopImage;
    juce::Image settingsImage;

    void playButtonClicked();
    void stopButtonClicked();
    void loopButtonClicked();
    void musicLibRootButtonClicked();
    void settingsButtonClicked();
    void openAudioSettings();
    void closeAudioSettings();

    void deleteMusicLibRoot();
    void updateMusicLibsComboBox();
    void onDefaultCrossFadeToggleChange();
    void onDefaultCrossFadeTextEditShow();
    void onDefaultCrossFadeTextEditHide();


    void createButtonImages();
    void volSliderValueChanged();
    void timeLineValueChanged(bool userChanged);
    void onCrossFadeCheckBoxChange();
    void onCrossFadeTextEditShow();
    void onCrossFadeTextEditHide(bool userChanged);

    void fileDoubleClicked(const juce::File& file);
    void selectionChanged() {};
    void fileClicked(const juce::File& file, const juce::MouseEvent& e) {};
    void browserRootChanged(const juce::File& newRoot);
    AudioFile* findFileInAllFiles(const juce::File& file);
    void setCrossFade(double time);

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void changeState(TransportState newState);
    void changeLoopmode(Loopmode newLoopmode);
    double timeStampToNumber(juce::String s);
    juce::String numberToTimeStamp(double n);
    void initTimeLine();
    void updateTimeLine();
    void setLoopTimeStamps(double loopStart, double loopEnd);
    void openFile(const juce::File& file);
    void saveAllSettingsToFile();
    void loadAllSettingsFromFile();
    void initAudioSettings();

    const double maxCrossFade = 20;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
