#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent
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
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    //==============================================================================
    // Your private member variables go here...


    float volume=1;


    juce::TextButton openButton;
    juce::TextButton playButton;
    juce::TextButton stopButton;
    juce::Slider volSlider;


    enum TransportState
    {
        Stopped,
        Starting,
        Playing,
        Pausing,
        Paused,
        Stopping
    };


    juce::AudioFormatManager formatManager;
    TransportState state;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::AudioSampleBuffer fileBuffer;
    const float *const *readPtrs = nullptr;
    int curPosition{};

    void openButtonClicked();
    void playButtonClicked();
    void stopButtonClicked();
    void volSliderValueChanged();

    void changeState(TransportState newState);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
