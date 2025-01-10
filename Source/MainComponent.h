#pragma once

#include <JuceHeader.h>
#include "TimeLine.h"
#include "AudioFile.h"






class FileBrowserCompHelper 
{
public:
    FileBrowserCompHelper() : thread("fileThread"),
        audioFileFilter("*.mp3;*.wav;*.flac;*.ogg;*.wmv;*.aif;*.aiff;*.asf;*.wma;*.wm;*.bwf", "*", "AudioFiles")
    {}

    juce::TimeSliceThread thread;
    const juce::WildcardFileFilter audioFileFilter;
};

class FileBrowserComp : public FileBrowserCompHelper, public juce::FileBrowserComponent
{
public:

    FileBrowserComp(): juce::FileBrowserComponent(juce::FileBrowserComponent::FileChooserFlags::openMode + juce::FileBrowserComponent::FileChooserFlags::canSelectFiles,
                                                    juce::File(), &audioFileFilter, nullptr)
    {
        //hacky way to "remove" the filenameBox+label beneath
        this->removeChildComponent(3);//label
        this->removeChildComponent(2);//box
    }


private:


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



class MusicLibViewContentComponent :public juce::Component{
public:
    MusicLibViewContentComponent() {

        setInterceptsMouseClicks(false, true);

        textLabel.setText("Music Librarys", juce::NotificationType::dontSendNotification);
        textLabel.setJustificationType(juce::Justification::centredTop);
        textLabel.setFont(juce::Font(20));
        textLabel.setInterceptsMouseClicks(false,false);
        addAndMakeVisible(textLabel);

        addAndMakeVisible(pathsCombo);

        delButton.setButtonText("delete");
        addAndMakeVisible(delButton);

        backButton.setButtonText("back");
        addAndMakeVisible(backButton);

        fb.flexDirection = juce::FlexBox::Direction::column;
        fb.flexWrap = juce::FlexBox::Wrap::noWrap;
        fb.justifyContent = juce::FlexBox::JustifyContent::spaceAround;
        fb.alignContent = juce::FlexBox::AlignContent::center;
        fb.alignItems = juce::FlexBox::AlignItems::stretch;
        

        fb.items.add(juce::FlexItem(textLabel)
            .withFlex(1, 1, 50)
            .withMaxHeight(50)
        );
        
        fbCombo.flexDirection = juce::FlexBox::Direction::row;
        fbCombo.flexWrap = juce::FlexBox::Wrap::noWrap;
        fbCombo.justifyContent = juce::FlexBox::JustifyContent::center;
        fbCombo.alignContent = juce::FlexBox::AlignContent::stretch;
        fbCombo.alignItems = juce::FlexBox::AlignItems::stretch;

        fbCombo.items.add(juce::FlexItem().withFlex(0.5, 1, 30));
        fbCombo.items.add(juce::FlexItem(pathsCombo)
            .withFlex(5, 2, 400)
            //.withMinHeight(20)
            //.withMaxHeight(60)
            .withMinWidth(50)
        );
        fbCombo.items.add(juce::FlexItem().withFlex(0.5, 1, 30)
        );


        fb.items.add(juce::FlexItem(fbCombo)
            //.withMargin(juce::FlexItem::Margin(20,50,20,50))
            .withMaxHeight(60)
            .withMinHeight(20)
            .withFlex(1,1,40)
        );

        fbButtons.flexDirection = juce::FlexBox::Direction::row;
        fbButtons.flexWrap = juce::FlexBox::Wrap::noWrap;
        fbButtons.justifyContent = juce::FlexBox::JustifyContent::center;
        fbButtons.alignItems = juce::FlexBox::AlignItems::stretch;

        fbButtons.items.add(juce::FlexItem().withFlex(0.5, 1, 30));
        fbButtons.items.add(juce::FlexItem(delButton)
            .withFlex(1, 0.25, 150)
            .withMaxWidth(150)
        );
        fbButtons.items.add(juce::FlexItem().withFlex(5, 1.5, 100));
        fbButtons.items.add(juce::FlexItem(backButton)
            .withFlex(1, 0.25, 150)
            .withMaxWidth(150)
        );
        fbButtons.items.add(juce::FlexItem().withFlex(0.5, 1, 30));

        fb.items.add(juce::FlexItem(fbButtons)
            .withMinHeight(20)
            .withMaxHeight(60)
            .withFlex(1,1,40)
        );
    }
    ~MusicLibViewContentComponent() {};

    juce::FlexBox fb;
    juce::FlexBox fbButtons;
    juce::FlexBox fbCombo;
    juce::Label textLabel;
    juce::ComboBox pathsCombo;
    juce::TextButton delButton;
    juce::TextButton backButton;


    void resized() {


        fb.performLayout(getLocalBounds());
    }

};

class MusicLibViewWindow :public juce::ResizableWindow {

public:
    MusicLibViewWindow(): juce::ResizableWindow("musicLibsWindow", true)
    {

        musicLibViewContentComponent.delButton.onClick = [this] {onDelButtonClicked(); };
        musicLibViewContentComponent.backButton.onClick = [this] {closeButtonPressed(); };

        setContentComponent(&musicLibViewContentComponent);

        setSize(500, 200);
        setCentreRelative(0.2f, 0.2f);
        setResizable(true, true);
        setDraggable(true);

    };
    ~MusicLibViewWindow() {};

    void closeButtonPressed() {

        exitModalState();
        setVisible(false);

    }

    MusicLibViewContentComponent musicLibViewContentComponent;


    std::function<void()> onDelButtonClicked;
};


//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent : public juce::AudioAppComponent,
                      public juce::FileBrowserListener,
                      public juce::ChangeListener
{
public:
    //==============================================================================
    MainComponent();
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
    ControlButton musicLibRootButton;
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

    std::unique_ptr<juce::FileChooser> chooser;

    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;
    juce::AudioSourceChannelInfo transitionChannelInfo;
    juce::AudioSampleBuffer transitionBuffer;
    double curSampleRate = 0;
    double curVolume=1;

    double crossFade = 0;
    juce::TimeSliceThread fileBufferThreat = juce::TimeSliceThread("fileBufferThreat");
    bool inTransition = false;
    double crossFadeProgress = 0;
    juce::int64 otherNextReadPos;
    int curPosition=0;

    double loopStartTime;
    double loopEndTime;
    double fadeStartTime;
    double fadeEndTime;

    FileBrowserComp fileBrowser{};
    std::vector<juce::File> musicLibRoots;
    std::vector<AudioFile> allFiles;
    AudioFile* currentFile=nullptr;

    MusicLibViewWindow musicLibViewWindow;

    juce::Image playImage;
    juce::Image pauseImage;
    juce::Image stopImage;
    juce::Image noLoopImage;
    juce::Image wholeLoopImage;
    juce::Image sectionLoopImage;
    juce::Image musicLibImage;
    juce::Image musicLibHighlightedImage;

    void playButtonClicked();
    void stopButtonClicked();
    void loopButtonClicked();
    void musicLibRootButtonLeftClicked();
    void musicLibRootButtonRightClicked();
    void deleteMusicLibRoot();
    void updateMusicLibsComboBox();

    void createButtonImages();
    void settingsButtonClicked();
    void volSliderValueChanged();
    void timeLineValueChanged(bool userChanged);
    void onCrossFadeCheckBoxChange();
    void onCrossFadeTextEditShow();
    void onCrossFadeTextEditHide();

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


    const double maxCrossFade = 20;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
