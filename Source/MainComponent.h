#pragma once

#include <JuceHeader.h>


class TimeLine : public juce::Slider,
                 public juce::Timer
{
public:
    TimeLine() {

        textFromValueFunction = [](double value) {
            int seconds = ((int)floor(value)) % 60;
            int minutes = value / 60;
            int hours = minutes / 60;
            if (hours > 0) { minutes = minutes % 60; }

            return  (hours>0 ? juce::String(hours)+":" : "") +
                    (hours > 0 && minutes<10 ? "0" : "") + juce::String(minutes) + ":" +
                    (seconds<10 ? "0":"") + juce::String(seconds);
            };

        onDragEnd = [this]() {
            DBG(getValue());
            onValueChange(true);
            mouseIsDragged=false;
            };

        onDragStart = [this]() {
            mouseIsDragged = true;
            };

    };

    ~TimeLine() {};
    std::function<void()> onTimerCallback;
    std::function<void(bool)> onValueChange;

    double guiRefreshTime = 100; //ms;
    bool mouseIsDragged=false;



private:
    void timerCallback() final { onTimerCallback(); };
    

};

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


    juce::TextButton playButton;
    juce::TextButton stopButton;
    juce::TextButton loopButton;
    juce::TextButton settingsButton;
    juce::Slider volSlider;

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


    std::unique_ptr<juce::FileChooser> chooser;

    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;
    double curSampleRate = 0;
    juce::TimeSliceThread fileBufferThreat = juce::TimeSliceThread("fileBufferThreat");


    FileBrowserComp fileBrowser{};

    void playButtonClicked();
    void stopButtonClicked();
    void loopButtonClicked();
    void settingsButtonClicked();
    void volSliderValueChanged();
    void timeLineValueChanged(bool userChanged);
    
    void fileDoubleClicked(const juce::File& file);
    void selectionChanged() {};
    void fileClicked(const juce::File& file, const juce::MouseEvent& e) {};
    void browserRootChanged(const juce::File& newRoot) {};

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void changeState(TransportState newState);

    void initTimeLine();
    void updateTimeLine();

    void openFile(const juce::File& file);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
