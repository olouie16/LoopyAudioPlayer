#pragma once

#include <JuceHeader.h>


class TimeLine : public juce::Slider,
                 public juce::Timer
{
public:
    TimeLine() {

        textFromValueFunction = [](double value) {
            int minutes = value / 60;
            int seconds = ((int)floor(value)) % 60;

            return juce::String(minutes) + ":" + (seconds<10 ? "0":"") + juce::String(seconds);
            };
    };

    ~TimeLine() {};
    std::function<void()> onTimerCallback;
    bool disableOnValueChanged = false;
private:
    void timerCallback() final { onTimerCallback(); };

};


//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent : public juce::AudioAppComponent
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


    float volume = 1;
    int curPosition{}; //in samples
    float sampleRate;
    float fileDuration; //in secs


    juce::TextButton openButton;
    juce::TextButton playButton;
    juce::TextButton stopButton;
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


    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::AudioSampleBuffer fileBuffer;
    std::vector<const float*> readPtrs = {};


    void openButtonClicked();
    void playButtonClicked();
    void stopButtonClicked();
    void volSliderValueChanged();
    void timeLineValueChanged();

    void changeState(TransportState newState);

    void initTimeLine();
    void updateTimeLine();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
