#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    setSize (600, 400);

    state = TransportState::Stopped;

    openButton.setButtonText("Open...");
    openButton.onClick = [this] { openButtonClicked(); };
    addAndMakeVisible(openButton);

    playButton.setButtonText("Play");
    playButton.onClick = [this] { playButtonClicked(); };
    playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
    playButton.setEnabled(false);
    addAndMakeVisible(playButton);

    stopButton.setButtonText("Stop");
    stopButton.onClick = [this] { stopButtonClicked(); };
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    stopButton.setEnabled(false);
    addAndMakeVisible(stopButton);


    volSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volSlider.setRange(0.0, 4.0, 0.01);
    volSlider.setTextBoxStyle(juce::Slider::TextBoxRight, true, 30, 30);
    volSlider.setSkewFactorFromMidPoint(1);
    volSlider.setValue(1.0);
    volSlider.onValueChange = [this]() {volSliderValueChanged(); };
    addAndMakeVisible(volSlider);


    formatManager.registerBasicFormats();

    setAudioChannels(2, 2);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));


}

void MainComponent::resized()
{
    // This is called when the MainComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.

    openButton.setBounds(20, getHeight() - 70, 50, 50);
    playButton.setBounds(90, getHeight() - 70, 50, 50);
    stopButton.setBounds(160, getHeight() - 70, 50, 50);
    volSlider.setBounds(230, getHeight() - 70, 300, 50);


}

void MainComponent::openButtonClicked()
{
    chooser = std::make_unique<juce::FileChooser>("Select a Wave file to play...",
                                                    juce::File{},
                                                    "*");

    auto chooserFlags = juce::FileBrowserComponent::openMode
        | juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
    {
        auto file = fc.getResult();

        if (file != juce::File{})
        {
            juce::AudioFormatReader* reader = formatManager.createReaderFor(file);

            if (reader != nullptr)
            {
                fileBuffer = juce::AudioSampleBuffer(2, reader->lengthInSamples);
                reader->read(&fileBuffer, 0, reader->lengthInSamples, 0, true, true);
                readPtrs = fileBuffer.getArrayOfReadPointers();
                playButton.setEnabled(true);
            }
        }
    });

}

void MainComponent::playButtonClicked()
{
    //updateLoopState(loopingToggle.getToggleState());
    if (state == Playing)
    {
        changeState(Paused);
    }
    else {
        changeState(Playing);
    }
}

void MainComponent::stopButtonClicked()
{
    changeState(Stopped);
}

void MainComponent::volSliderValueChanged()
{
    volume = volSlider.getValue();

}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{


}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    //auto* device = deviceManager.getCurrentAudioDevice();

    //auto activeInputChannels = device->getActiveInputChannels();
    //auto activeOutputChannels = device->getActiveOutputChannels();

    //int maxInputChannels = activeInputChannels.countNumberOfSetBits();
    //int maxOutputChannels = activeOutputChannels.countNumberOfSetBits();


    bufferToFill.clearActiveBufferRegion();


    if (state == Playing)
    {
        for (int channel = 0; channel < 2; channel++)
        {

            auto writePtr = bufferToFill.buffer->getWritePointer(channel, bufferToFill.startSample);
            for (int sample = 0; sample < bufferToFill.numSamples; sample++)
            {
                if (curPosition + sample < fileBuffer.getNumSamples())
                {
                    writePtr[sample] = readPtrs[channel][curPosition + sample] * volume;
                }
                else
                { 
                    break;
                }
            }
        }

        curPosition += bufferToFill.numSamples;
        if (curPosition >= fileBuffer.getNumSamples()) {
            curPosition = 0;
        }
    }
}

void MainComponent::releaseResources()
{
  
}


void MainComponent::changeState(TransportState newState)
{
    if (state != newState)
    {
        state = newState;

        switch (state)
        {
        case Stopped:
            playButton.setEnabled(true);
            stopButton.setEnabled(false);
            playButton.setButtonText("Play");
            playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
            curPosition = 0;
            break;
        case Paused:
            playButton.setButtonText("Play");
            playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
            break;
        case Starting:
            playButton.setButtonText("Pause");
            changeState(Playing);
            break;

        case Playing:
            playButton.setButtonText("Pause");
            playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::orange);
            stopButton.setEnabled(true);
            break;

        case Stopping:
            playButton.setButtonText("Play");
            playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
            break;
        }
    }
}
















