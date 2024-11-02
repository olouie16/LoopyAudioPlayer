#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    setSize (1000, 700);



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

    loopButton.setButtonText("Loop");
    loopButton.onClick = [this] { loopButtonClicked(); };
    loopButton.setColour(juce::TextButton::buttonColourId, juce::Colours::blue);
    loopButton.setEnabled(true);
    addAndMakeVisible(loopButton);

    settingsButton.setButtonText("Settings");
    settingsButton.onClick = [this] { settingsButtonClicked(); };
    settingsButton.setColour(juce::TextButton::buttonColourId, juce::Colours::lightgrey);
    settingsButton.setEnabled(true);
    addAndMakeVisible(settingsButton);


    volSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volSlider.setRange(0.0, 4.0, 0.0001);
    volSlider.setTextBoxStyle(juce::Slider::TextBoxRight, true, 30, 30);
    volSlider.setSkewFactorFromMidPoint(1);
    volSlider.textFromValueFunction = [](double value) {return juce::String(value, 2 ,false); };
    volSlider.setValue(1);
    volSlider.onValueChange = [this]() {volSliderValueChanged(); };
    addAndMakeVisible(volSlider);

    timeLine.setSliderStyle(juce::Slider::LinearHorizontal);
    timeLine.setRange(0.0,0.01,0.01);
    timeLine.setValue(0.0);
    timeLine.setTextBoxIsEditable(false);
    timeLine.onValueChange = [this](bool userChanged=false) {timeLineValueChanged(userChanged); };
    timeLine.onTimerCallback = [this]() {updateTimeLine(); };
    timeLine.onLoopMarkerChange = [this](double left, double right) {setLoopTimeStamps(left, right); };
    timeLine.startTimer(timeLine.guiRefreshTime);
    addAndMakeVisible(timeLine);

    fileBrowser.addListener(this);
    addAndMakeVisible(fileBrowser);

    formatManager.registerBasicFormats();
    transportSource.addChangeListener(this);

    fileBufferThreat.startThread();
    setAudioChannels(2, 2);

    changeState(TransportState::Stopped);
    playButton.setEnabled(false);
    changeLoopmode(notLooping);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
    fileBufferThreat.stopThread(10000);
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));


}

void MainComponent::resized()
{

    playButton.setBounds(20, getHeight() - 70, 50, 50);
    stopButton.setBounds(playButton.getRight() + 20, getHeight() - 70, 50, 50);
    loopButton.setBounds(stopButton.getRight() + 20, getHeight() - 70, 50, 50);
    volSlider.setBounds(loopButton.getRight() + 20, getHeight() - 70, std::min(400, getWidth() - 60 - 70 - loopButton.getRight() + 20), 50);
    settingsButton.setBounds(volSlider.getRight() + 20, getHeight() - 70, 50, 50);
    timeLine.setBounds(20,getHeight()-140,getWidth()-40,50);
    fileBrowser.setBounds(20, 20, getWidth()-40, timeLine.getPosition().y - 20);
}

void MainComponent::playButtonClicked()
{
    //updateLoopState(loopingToggle.getToggleState());
    if (state == Playing)
    {
        changeState(Pausing);
    }
    else {
        changeState(Starting);
        //changeState(Playing);
    }
}

void MainComponent::stopButtonClicked()
{
    changeState(Stopping);
}

void MainComponent::loopButtonClicked()
{
    switch (loopmode)
    {
    case notLooping:
        changeLoopmode(loopWhole);
        break;
    case loopWhole:
        changeLoopmode(loopSection);
        break;
    case loopSection:
        changeLoopmode(notLooping);
        break;
    default:
        changeLoopmode(notLooping);
        break;
    }
}

void MainComponent::settingsButtonClicked()
{


}

void MainComponent::volSliderValueChanged()
{
    curVolume = volSlider.getValue();
    transportSource.setGain(curVolume);
}

void MainComponent::timeLineValueChanged(bool userChanged)
{
    if (userChanged) {

        double newTime = timeLine.getValue();

        if (loopmode == loopSection) {
            if (newTime > fadeStartTime) {
                if (newTime > loopEndTime) {
                    //after loopSection
                    changeLoopmode(loopWhole);
                    inTransition = false;
                }
                else {
                    //in crossFade
                    inTransition = true;
                    double timeAlreadyInCrossFade = newTime - fadeStartTime;
                    crossFadeProgress = timeAlreadyInCrossFade / crossFade;
                    otherNextReadPos = (loopStartTime + timeAlreadyInCrossFade) * curSampleRate;
                }
            }
        }

        transportSource.setPosition(newTime);
        timeLine.startTimer(timeLine.guiRefreshTime);
    }
    else {
        updateTimeLine();
    }

}


void MainComponent::fileDoubleClicked(const juce::File& file)
{
    openFile(file);
    changeState(Starting);
}



void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    //transitionTransportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    curSampleRate = sampleRate;
    transitionBuffer = juce::AudioSampleBuffer(2, (crossFade+0.1) * sampleRate + 2 * samplesPerBlockExpected);
    transitionChannelInfo = juce::AudioSourceChannelInfo(transitionBuffer);

}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{


    if (readerSource.get() == nullptr)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }



    if (inTransition)
    {


        if (transportSource.getCurrentPosition()+ (bufferToFill.numSamples/curSampleRate) > loopEndTime) {
            //end of crossfade
            inTransition = false;

            double timeAlreadyOutOfLoop = transportSource.getCurrentPosition() + (bufferToFill.numSamples / curSampleRate) - loopEndTime;
            int numSamplesAlreadyOutOfLoop = timeAlreadyOutOfLoop * curSampleRate;

            transitionChannelInfo.numSamples = bufferToFill.numSamples;
            transportSource.getNextAudioBlock(transitionChannelInfo);

            transportSource.setNextReadPosition(otherNextReadPos);
            transportSource.getNextAudioBlock(bufferToFill);

            bufferToFill.buffer->applyGainRamp(bufferToFill.startSample, transitionChannelInfo.numSamples - numSamplesAlreadyOutOfLoop, crossFadeProgress, 1);
            //transitionChannelInfo.buffer->applyGainRamp(transitionChannelInfo.startSample, numSamplesAlreadyInLoop, 0, crossFadeProgress);

            for (int channel = 0; channel < bufferToFill.buffer->getNumChannels(); channel++) {
                auto readPtr = transitionChannelInfo.buffer->getReadPointer(channel, transitionChannelInfo.startSample);
                bufferToFill.buffer->addFromWithRamp(channel, bufferToFill.startSample, readPtr, transitionChannelInfo.numSamples - numSamplesAlreadyOutOfLoop, 1-crossFadeProgress, 0);
            }
            crossFadeProgress = 0;
            transitionChannelInfo.startSample = 0;
            //transportSource.setNextReadPosition(transitionTransportSource.getNextReadPosition());
        }
        else {
            //while crossFade
            transportSource.getNextAudioBlock(bufferToFill);
            double gainStart = crossFadeProgress;
            crossFadeProgress += (bufferToFill.numSamples / curSampleRate) / crossFade;

            transitionChannelInfo.numSamples = bufferToFill.numSamples;
            juce::int64 readPos = transportSource.getNextReadPosition();
            transportSource.setNextReadPosition(otherNextReadPos);
            transportSource.getNextAudioBlock(transitionChannelInfo);

            bufferToFill.buffer->applyGainRamp(bufferToFill.startSample, bufferToFill.numSamples, 1 - gainStart, 1- crossFadeProgress);

            for (int channel = 0; channel < bufferToFill.buffer->getNumChannels(); channel++) {
                auto readPtr = transitionChannelInfo.buffer->getReadPointer(channel, transitionChannelInfo.startSample);
                bufferToFill.buffer->addFromWithRamp(channel, bufferToFill.startSample, readPtr, transitionChannelInfo.numSamples, gainStart, crossFadeProgress);
            }


            transitionChannelInfo.startSample += transitionChannelInfo.numSamples;
            otherNextReadPos = transportSource.getNextReadPosition();
            transportSource.setNextReadPosition(readPos);

        }


    }
    else 
    {

        transportSource.getNextAudioBlock(bufferToFill);

        if (transportSource.getCurrentPosition() > fadeStartTime) {
            if (crossFade > 0) {

                //begin of crossFade
                inTransition = true;

                double timeAlreadyInLoop = transportSource.getCurrentPosition() - fadeStartTime;
                int numSamplesAlreadyInLoop = timeAlreadyInLoop * curSampleRate;

                crossFadeProgress = (timeAlreadyInLoop/crossFade);

                transitionChannelInfo.startSample = 0;
                transitionChannelInfo.numSamples = numSamplesAlreadyInLoop;
                juce::int64 nextReadPos = transportSource.getNextReadPosition();
                transportSource.setPosition(loopStartTime);
                transportSource.getNextAudioBlock(transitionChannelInfo);

                bufferToFill.buffer->applyGainRamp(bufferToFill.startSample + (bufferToFill.numSamples - numSamplesAlreadyInLoop), numSamplesAlreadyInLoop, 1, 1 - crossFadeProgress);
                //transitionChannelInfo.buffer->applyGainRamp(transitionChannelInfo.startSample, numSamplesAlreadyInLoop, 0, crossFadeProgress);
                for (int channel = 0; channel < bufferToFill.buffer->getNumChannels();channel++) {
                    auto readPtr = transitionChannelInfo.buffer->getReadPointer(channel);
                    bufferToFill.buffer->addFromWithRamp(channel, bufferToFill.startSample, readPtr, numSamplesAlreadyInLoop, 0, crossFadeProgress);
                }
                transitionChannelInfo.startSample += numSamplesAlreadyInLoop;
                otherNextReadPos = transportSource.getNextReadPosition();
            
                transportSource.setNextReadPosition(nextReadPos);
            }
            else {
                //only jump, no crossfade
                transportSource.setPosition(loopStartTime);

            }
        }
    }


}

void MainComponent::releaseResources()
{
    transportSource.releaseResources();

}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &transportSource)
    {
        if (!inTransition) {
            if (transportSource.isPlaying())
                changeState(Playing);
            else if ((state == Stopping) || (state == Playing))
                changeState(Stopped);
            else if (state == Pausing)
                changeState(Paused);
        }
    }
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
            transportSource.setPosition(0.0);
            break;
        case Pausing:
            transportSource.stop();
            break;
        case Paused:
            playButton.setButtonText("Play");
            playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
            break;
        case Starting:
            playButton.setButtonText("Pause");
            transportSource.start();
            break;

        case Playing:
            playButton.setButtonText("Pause");
            playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::orange);
            stopButton.setEnabled(true);
            break;

        case Stopping:
            playButton.setButtonText("Play");
            playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
            transportSource.stop();
            break;
        }
    }
}

void MainComponent::changeLoopmode(Loopmode newLoopmode) {

        loopmode = newLoopmode;

        switch (loopmode) 
        {
        case notLooping:
            
            loopButton.setColour(juce::TextButton::textColourOffId, juce::Colours::lightcyan);
            loopButton.setButtonText("no Loop");
            timeLine.hideLoopMarkers();
            if(readerSource!=nullptr)
                readerSource->setLooping(false);
            
            //inf so loop or fade will be initiated
            loopStartTime = -INFINITY;
            loopEndTime = INFINITY;
            fadeStartTime = INFINITY;
            fadeEndTime = -INFINITY;
            break;
        case loopWhole:
            
            loopButton.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
            loopButton.setButtonText("Loop Whole");
            timeLine.hideLoopMarkers();
            if (readerSource != nullptr)
                readerSource->setLooping(true);

            loopStartTime = 0;
            loopEndTime = transportSource.getLengthInSeconds();
            fadeStartTime = loopEndTime-crossFade;
            fadeEndTime = loopStartTime+crossFade;
            break;
        case loopSection:
            
            loopButton.setColour(juce::TextButton::textColourOffId, juce::Colours::orange);
            loopButton.setButtonText("Loop Region");
            timeLine.showLoopMarkers();
            if (readerSource != nullptr)
                readerSource->setLooping(true);
            
            
            //later with AudioFile Class saved timeStamps
            setLoopTimeStamps(timeLine.getLeftLoopTimestamp(), timeLine.getRightLoopTimestamp());
            break;
        }
}

void MainComponent::initTimeLine() {
    timeLine.setRange(0.0, transportSource.getLengthInSeconds(), 0.01);
    timeLine.updateLoopMarkers();
    updateTimeLine();
}

void MainComponent::updateTimeLine()
{

    if (!timeLine.mouseIsDragged) {

        if (transportSource.getLengthInSeconds() > 0 && transportSource.getTotalLength() > 0) {
            timeLine.setValue((float)transportSource.getNextReadPosition()/curSampleRate);
        }
        else
        {
            timeLine.setValue(0);
        }
    }
}

void MainComponent::setLoopTimeStamps(double loopStart, double loopEnd) {

    double curTime = transportSource.getCurrentPosition();
    if (loopmode == loopSection && curTime > loopEnd)
        changeLoopmode(loopWhole);

    if (loopmode == loopWhole) {

        loopStartTime = loopStart;
        loopEndTime = loopEnd;

        fadeStartTime = loopEndTime - crossFade;
        fadeEndTime = loopStartTime + crossFade;
    }
}

void MainComponent::openFile(const juce::File& file)
{

    if (file != juce::File{})
    {
        juce::AudioFormatReader* reader = formatManager.createReaderFor(file);

        if (reader != nullptr)
        {
            auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
            //transportSource.setSource(newSource.get(), 6000*48000, &fileBufferThreat, reader->sampleRate);
            transportSource.setSource(newSource.get(),0, nullptr, reader->sampleRate);


            playButton.setEnabled(true);
            readerSource.reset(newSource.release());
            
            playButton.setEnabled(true);

            initTimeLine();
            //changeState(state);
            changeLoopmode(loopmode);



            //load saved loopTimings here
            fadeStartTime = transportSource.getLengthInSeconds() + 1;//for now just never loop until timings are set

        }
    }

}







