#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    setSize (1000, 700);

    createButtonImages();

    playButton.setImage(playImage);
    playButton.onClick = [this] { playButtonClicked(); };
    playButton.setEnabled(false);
    addAndMakeVisible(playButton);

    stopButton.setImage(stopImage);
    stopButton.onClick = [this] { stopButtonClicked(); };
    stopButton.setEnabled(false);
    addAndMakeVisible(stopButton);

    loopButton.onClick = [this] { loopButtonClicked(); };
    loopButton.setEnabled(true);
    addAndMakeVisible(loopButton);

    musicLibRootButton.setImage(musicLibImage);
    musicLibRootButton.onLeftClick = [this] {musicLibRootButtonLeftClicked(); };
    musicLibRootButton.onRightClick = [this] {musicLibRootButtonRightClicked(); };
    musicLibRootButton.setEnabled(true);
    addAndMakeVisible(musicLibRootButton);


    volSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volSlider.setRange(0.0, 4.0, 0.0001);
    volSlider.setTextBoxStyle(juce::Slider::TextBoxRight, true, 30, 30);
    volSlider.setSkewFactorFromMidPoint(1);
    volSlider.textFromValueFunction = [](double value) {return juce::String(value, 2 ,false); };
    volSlider.setValue(1);
    volSlider.onValueChange = [this]() {volSliderValueChanged(); };
    addAndMakeVisible(volSlider);

    crossFadeCheckBox.onStateChange = [this]() {onCrossFadeCheckBoxChange(); };
    crossFadeCheckBox.setButtonText("Cross Fade");
    addAndMakeVisible(crossFadeCheckBox);

    crossFadeLabel.setText("1",juce::NotificationType::dontSendNotification);
    crossFadeLabel.setEditable(true);
    crossFadeLabel.onEditorShow = [this]() {onCrossFadeTextEditShow(); };
    crossFadeLabel.onEditorHide = [this]() {onCrossFadeTextEditHide(); };
    addAndMakeVisible(crossFadeLabel);

    crossFadeUnit.setText("sek", juce::NotificationType::dontSendNotification);
    addAndMakeVisible(crossFadeUnit);

    timeLine.setSliderStyle(juce::Slider::LinearHorizontal);
    timeLine.setRange(0.0,0.01,0.01);
    timeLine.setValue(0.0);
    //timeLine.setTextBoxIsEditable(false);
    timeLine.onValueChange = [this](bool userChanged=false) {timeLineValueChanged(userChanged); };
    timeLine.onTimerCallback = [this]() {updateTimeLine(); };
    timeLine.onLoopMarkerChange = [this](double left, double right) {setLoopTimeStamps(left, right); };
    timeLine.startTimer(timeLine.guiRefreshTime);
    addAndMakeVisible(timeLine);

    fileBrowser.addListener(this);
    addAndMakeVisible(fileBrowser);

    //musicLibViewWindow.setCentreRelative(0.5f, 0.5f);
    musicLibViewWindow.onDelButtonClicked = [this] {deleteMusicLibRoot(); };
    addChildComponent(musicLibViewWindow);

    formatManager.registerBasicFormats();
    transportSource.addChangeListener(this);

    //fileBufferThreat.startThread();
    setAudioChannels(2, 2);

    changeState(TransportState::Stopped);
    playButton.setEnabled(false);
    changeLoopmode(notLooping);

    musicLibs = std::vector<juce::File>();
    allFiles = std::vector<AudioFile>();

    loadAllSettingsFromFile();
}

MainComponent::~MainComponent()
{
    saveAllSettingsToFile();
    shutdownAudio();
    //fileBufferThreat.stopThread(10000);
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
    timeLine.setBounds(20,getHeight()-140,getWidth()-60,50);
    crossFadeCheckBox.setBounds(volSlider.getRight() + 5, volSlider.getPosition().y, 80, 50);
    crossFadeLabel.setBounds(crossFadeCheckBox.getRight()+3, getHeight() - 60, 40, 30);
    crossFadeUnit.setBounds(crossFadeLabel.getRight(), crossFadeLabel.getPosition().y, 40, 30);
    musicLibRootButton.setBounds(crossFadeUnit.getRight() + 20, getHeight() - 70, 50, 50);
    fileBrowser.setBounds(20, 20, getWidth()-40, timeLine.getPosition().y - 20);
}

void MainComponent::playButtonClicked()
{
    if (state == Playing)
    {
        changeState(Pausing);
    }
    else {
        changeState(Starting);
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

void MainComponent::musicLibRootButtonLeftClicked() {
    juce::File newRoot = fileBrowser.getRoot();

    if (newRoot.isDirectory()) {

        
        //remove if already in List
        bool alreadyInList = false;
        for (int i = 0; i < musicLibs.size(); i++) {
            if (newRoot == musicLibs[i]) {
                alreadyInList = true;
                musicLibs.erase(musicLibs.begin() + i);
                musicLibRootButton.setImage(musicLibImage);
                break;
            }
        }

        //Add new musicLibRoot
        if (!alreadyInList) {
            musicLibs.push_back(newRoot);
            musicLibRootButton.setImage(musicLibHighlightedImage);
        }

    }
    updateMusicLibsComboBox();
}

void MainComponent::musicLibRootButtonRightClicked()
{
    
    musicLibViewWindow.setVisible(true);
    musicLibViewWindow.enterModalState();

}

void MainComponent::deleteMusicLibRoot() {
    int index = musicLibViewWindow.musicLibViewContentComponent.pathsCombo.getSelectedId()-1;
    musicLibs.erase(musicLibs.begin()+index);
    updateMusicLibsComboBox();
    browserRootChanged(fileBrowser.getRoot());
}

void MainComponent::updateMusicLibsComboBox()
{
    juce::ComboBox* cb = &musicLibViewWindow.musicLibViewContentComponent.pathsCombo;
    cb->clear();
    for (int i = 0; i < musicLibs.size();i++) {
        cb->addItem(musicLibs[i].getFullPathName(), i+1);
    }

}

void MainComponent::createButtonImages()
{

    int borderSize = 64;
    int imageSize = 512;

    float outline = 15;

    juce::Colour noLoopCol = juce::Colours::lightcyan;
    juce::Colour wholeLoopCol = juce::Colours::blue;
    juce::Colour sectionLoopCol = juce::Colours::orange;

    juce::Colour musicLibNormalCol = juce::Colours::lightcyan;
    juce::Colour musicLibHighlightedCol = juce::Colours::gold;

    playImage = juce::Image(juce::Image::ARGB, imageSize, imageSize, true);
    juce::Graphics playGraphics(playImage);
    pauseImage = juce::Image(juce::Image::ARGB, imageSize, imageSize, true);
    juce::Graphics pauseGraphics(pauseImage);
    stopImage = juce::Image(juce::Image::ARGB, imageSize, imageSize, true);
    juce::Graphics stopGraphics(stopImage);
    noLoopImage = juce::Image(juce::Image::ARGB, imageSize, imageSize, true);
    juce::Graphics noLoopGraphics(noLoopImage);
    wholeLoopImage = juce::Image(juce::Image::ARGB, imageSize, imageSize, true);
    juce::Graphics wholeLoopGraphics(wholeLoopImage);
    sectionLoopImage = juce::Image(juce::Image::ARGB, imageSize, imageSize, true);
    juce::Graphics sectionLoopGraphics(sectionLoopImage);
    musicLibImage = juce::Image(juce::Image::ARGB, imageSize, imageSize, true);
    juce::Graphics musicLibGraphics(musicLibImage);
    musicLibHighlightedImage = juce::Image(juce::Image::ARGB, imageSize, imageSize, true);
    juce::Graphics musicLibHighlightedGraphics(musicLibHighlightedImage);

    imageSize = imageSize - 2 * borderSize;

    juce::Path playShape;
    float xOffset = imageSize * (1 - (sqrt(3) / 2))/2;
    playShape.addTriangle(xOffset, 0,
                        xOffset, imageSize,
                        imageSize * (sqrt(3) / 2) + xOffset, imageSize / 2);

    playShape = playShape.createPathWithRoundedCorners(0.1 * imageSize);

    playGraphics.setOrigin(borderSize, borderSize);
    playGraphics.setColour(juce::Colours::green);
    playGraphics.fillPath(playShape);
    playGraphics.setColour(juce::Colours::black);
    playGraphics.strokePath(playShape, juce::PathStrokeType::PathStrokeType(outline));

    
    juce::Path pauseShape;
    pauseShape.addRoundedRectangle(0, 0, 0.33 * imageSize, imageSize, 0.1 * imageSize);
    pauseShape.addRoundedRectangle(0.66*imageSize, 0, 0.33 * imageSize, imageSize, 0.1 * imageSize);

    pauseGraphics.setOrigin(borderSize, borderSize);
    pauseGraphics.setColour(juce::Colours::yellow);
    pauseGraphics.fillPath(pauseShape);
    pauseGraphics.setColour(juce::Colours::black);
    pauseGraphics.strokePath(pauseShape, juce::PathStrokeType::PathStrokeType(outline));


    juce::Path stopShape;
    stopShape.addRoundedRectangle(0, 0, imageSize, imageSize, 0.1 * imageSize);

    stopGraphics.setOrigin(borderSize, borderSize);
    stopGraphics.setColour(juce::Colours::red);
    stopGraphics.fillPath(stopShape);
    stopGraphics.setColour(juce::Colours::black);
    stopGraphics.strokePath(stopShape, juce::PathStrokeType::PathStrokeType(outline));


    juce::Path loopShape;
    float tailAngle = 2.4 * juce::float_Pi;
    float headAngle = 1.6 * juce::float_Pi;
    float loopArrowWidth = 0.15 * imageSize;
    float outerRadius = 0.7 * imageSize / 2;
    float innerRadius = outerRadius - loopArrowWidth;

    loopShape.startNewSubPath(juce::Point<float>(0, -outerRadius).rotatedAboutOrigin<float>(tailAngle));
    loopShape.addCentredArc(0, 0, outerRadius, outerRadius, 0, tailAngle, headAngle);
    loopShape.lineTo(juce::Point<float>(0, -(outerRadius + loopArrowWidth)).rotatedAboutOrigin<float>(headAngle));
    loopShape.lineTo(juce::Point<float>(-loopArrowWidth, -(outerRadius+innerRadius) / 2).rotatedAboutOrigin<float>(headAngle));
    loopShape.lineTo(juce::Point<float>(0, -(innerRadius - loopArrowWidth)).rotatedAboutOrigin<float>(headAngle));
    loopShape.lineTo(juce::Point<float>(0, -innerRadius).rotatedAboutOrigin<float>(headAngle));
    loopShape.addCentredArc(0, 0, innerRadius, innerRadius, 0, headAngle, tailAngle);
    //loopPath.lineTo(juce::Point<float>(0, -outerRadius).rotatedAboutOrigin<float>(tailAngle));

    loopShape.closeSubPath();
    loopShape.addPath(loopShape, juce::AffineTransform::rotation(juce::float_Pi));

    juce::Path noLoopShape;
    noLoopShape.addRectangle(- outerRadius - loopArrowWidth, -0.05 * imageSize, 2*(outerRadius + loopArrowWidth), 0.1 * imageSize);
    

    noLoopGraphics.setOrigin(imageSize/2+borderSize, imageSize / 2 + borderSize);
    noLoopGraphics.setColour(noLoopCol.darker(0.2));
    noLoopGraphics.fillPath(loopShape);
    noLoopGraphics.setColour(juce::Colours::black);
    noLoopGraphics.strokePath(loopShape, juce::PathStrokeType::PathStrokeType(outline));

    noLoopGraphics.setColour(juce::Colours::red);
    noLoopGraphics.fillPath(noLoopShape, juce::AffineTransform::rotation(-0.25 * juce::float_Pi));
    noLoopGraphics.setColour(juce::Colours::black);
    noLoopGraphics.strokePath(noLoopShape, juce::PathStrokeType::PathStrokeType(0.5*outline), juce::AffineTransform::rotation(-0.25 * juce::float_Pi));

    juce::Path wholeLoopShape;
    wholeLoopShape.addRoundedRectangle(0, 0, 0.2 * imageSize, imageSize, 0.025*imageSize);
    wholeLoopShape.addRoundedRectangle(0.8*imageSize, 0, 0.2 * imageSize, imageSize, 0.025*imageSize);


    wholeLoopGraphics.setOrigin(borderSize, borderSize);
    wholeLoopGraphics.setColour(juce::Colours::blue);
    wholeLoopGraphics.fillPath(wholeLoopShape);
    wholeLoopGraphics.setColour(juce::Colours::black);
    wholeLoopGraphics.strokePath(wholeLoopShape, juce::PathStrokeType::PathStrokeType(outline));

    wholeLoopGraphics.setOrigin(imageSize / 2, imageSize / 2);
    wholeLoopGraphics.setColour(noLoopCol.interpolatedWith(wholeLoopCol ,0.6));
    wholeLoopGraphics.fillPath(loopShape);
    wholeLoopGraphics.setColour(juce::Colours::black);
    wholeLoopGraphics.strokePath(loopShape, juce::PathStrokeType::PathStrokeType(outline));


    juce::Image loopMarker = timeLine.getActiveLoopMarkerIcon();
    sectionLoopGraphics.setOrigin(borderSize, borderSize);
    sectionLoopGraphics.drawImage(loopMarker, juce::Rectangle<float>(0, 0, 0.2 * imageSize, imageSize), juce::RectanglePlacement::xLeft | juce::RectanglePlacement::yTop);
    sectionLoopGraphics.drawImage(loopMarker, juce::Rectangle<float>(0.8 * imageSize, 0, 0.2*imageSize, imageSize), juce::RectanglePlacement::xRight | juce::RectanglePlacement::yTop);

    sectionLoopGraphics.setOrigin(imageSize / 2, imageSize / 2);
    sectionLoopGraphics.setColour(noLoopCol.interpolatedWith(sectionLoopCol, 0.5));
    sectionLoopGraphics.fillPath(loopShape);
    sectionLoopGraphics.setColour(juce::Colours::black);
    sectionLoopGraphics.strokePath(loopShape, juce::PathStrokeType::PathStrokeType(outline));

    float topDiff = 0.1 * imageSize;
    float topBottomGab = 0.1 * imageSize;
    juce::Path musicLibShape;
    musicLibShape.startNewSubPath(0.5 * imageSize, topDiff + topBottomGab);
    musicLibShape.lineTo(imageSize, topDiff + topBottomGab);
    musicLibShape.lineTo(imageSize, imageSize - topBottomGab);
    musicLibShape.lineTo(0, imageSize- topBottomGab);
    musicLibShape.lineTo(0, topBottomGab);
    musicLibShape.lineTo(0.3*imageSize, topBottomGab);
    musicLibShape = musicLibShape.createPathWithRoundedCorners(0.1 * imageSize);
    musicLibShape.closeSubPath();

    musicLibGraphics.setOrigin(borderSize, borderSize);
    musicLibGraphics.setColour(musicLibNormalCol);
    musicLibGraphics.fillPath(musicLibShape);
    musicLibGraphics.setColour(juce::Colours::black);
    musicLibGraphics.strokePath(musicLibShape, juce::PathStrokeType::PathStrokeType(outline));

    musicLibHighlightedGraphics.setOrigin(borderSize, borderSize);
    musicLibHighlightedGraphics.setColour(musicLibHighlightedCol);
    musicLibHighlightedGraphics.fillPath(musicLibShape);
    musicLibHighlightedGraphics.setColour(juce::Colours::black);
    musicLibHighlightedGraphics.strokePath(musicLibShape, juce::PathStrokeType::PathStrokeType(outline));

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

        if (loopmode == fakeLoopSection && newTime < loopEndTime) {
            changeLoopmode(loopSection);
        }

        if (loopmode == loopSection) {
            if (newTime > fadeStartTime) {
                if (newTime > loopEndTime) {
                    //after loopSection
                    changeLoopmode(fakeLoopSection);
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
            else {
                inTransition = false;
                crossFadeProgress = 0;
            }
        }



        transportSource.setPosition(newTime);
        timeLine.startTimer(timeLine.guiRefreshTime);
    }
    else {
        updateTimeLine();
    }

}

void MainComponent::onCrossFadeCheckBoxChange()
{
    if (crossFadeCheckBox.getToggleState()) {
        setCrossFade(timeStampToNumber(crossFadeLabel.getText()));
    }
    else {
        setCrossFade(0);
    }
}

void MainComponent::onCrossFadeTextEditShow() {
    juce::TextEditor* edit = crossFadeLabel.getCurrentTextEditor();
    edit->setMultiLine(false);
    edit->setInputFilter(&crossFadeEditFilter, false);
}

void MainComponent::onCrossFadeTextEditHide() {
    juce::String text = crossFadeLabel.getText();
    double n = timeStampToNumber(text);
    if (n > maxCrossFade) {
        n = maxCrossFade;
        crossFadeLabel.setText(juce::String(maxCrossFade), juce::NotificationType::dontSendNotification);
    }
    if (crossFadeCheckBox.getToggleState()) {
        setCrossFade(n);
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
    curSampleRate = sampleRate;
    transitionBuffer = juce::AudioSampleBuffer(2, maxCrossFade * sampleRate + 2*samplesPerBlockExpected);
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

            for (int channel = 0; channel < bufferToFill.buffer->getNumChannels(); channel++) {
                auto readPtr = transitionChannelInfo.buffer->getReadPointer(channel, transitionChannelInfo.startSample);
                bufferToFill.buffer->addFromWithRamp(channel, bufferToFill.startSample, readPtr, transitionChannelInfo.numSamples - numSamplesAlreadyOutOfLoop, 1-crossFadeProgress, 0);
            }
            crossFadeProgress = 0;
            transitionChannelInfo.startSample = 0;
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
        bool fadeStartTimeReachedInNextBlock = false;
        if (transportSource.getCurrentPosition() + bufferToFill.numSamples/curSampleRate >= fadeStartTime) { fadeStartTimeReachedInNextBlock = true; }

        transportSource.getNextAudioBlock(bufferToFill);

        if (fadeStartTimeReachedInNextBlock) {
            if (crossFade > 0) {

                //begin of crossFade
                inTransition = true;

                double timeAlreadyInLoop = transportSource.getCurrentPosition() - fadeStartTime;
                int numSamplesAlreadyInLoop = timeAlreadyInLoop * curSampleRate;

                crossFadeProgress = (timeAlreadyInLoop/crossFade);

                //in rare case of switching to regionLoop mode while playhead should already be in transition
                float startGain, endGain;
                int offset;
                if (numSamplesAlreadyInLoop >= bufferToFill.numSamples) {
                    startGain = 1;
                    endGain = 1 - crossFadeProgress;
                    offset = (numSamplesAlreadyInLoop - bufferToFill.numSamples)/curSampleRate;
                    numSamplesAlreadyInLoop = bufferToFill.numSamples;
                }
                else {
                    startGain = 1;
                    endGain = 1 - crossFadeProgress;
                    offset = 0;
                }

                transitionChannelInfo.startSample = 0;
                transitionChannelInfo.numSamples = numSamplesAlreadyInLoop;
                juce::int64 nextReadPos = transportSource.getNextReadPosition();
                transportSource.setPosition(loopStartTime+offset);
                transportSource.getNextAudioBlock(transitionChannelInfo);

                bufferToFill.buffer->applyGainRamp(bufferToFill.startSample + (bufferToFill.numSamples - numSamplesAlreadyInLoop), numSamplesAlreadyInLoop, startGain, endGain);
                for (int channel = 0; channel < bufferToFill.buffer->getNumChannels();channel++) {
                    auto readPtr = transitionChannelInfo.buffer->getReadPointer(channel);
                    bufferToFill.buffer->addFromWithRamp(channel, bufferToFill.startSample, readPtr, numSamplesAlreadyInLoop, 1-startGain, 1-endGain);
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
            playButton.setImage(playImage);
            playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
            transportSource.setPosition(0.0);
            timeLine.setClickableTimeStamp(true);
            break;
        case Pausing:
            transportSource.stop();
            timeLine.setClickableTimeStamp(true);
            break;
        case Paused:
            playButton.setImage(playImage);
            playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
            timeLine.setClickableTimeStamp(true);
            break;
        case Starting:
            playButton.setImage(pauseImage);
            transportSource.start();
            timeLine.setClickableTimeStamp(false);
            break;

        case Playing:
            playButton.setImage(pauseImage);
            playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::orange);
            stopButton.setEnabled(true);
            timeLine.setClickableTimeStamp(false);
            break;

        case Stopping:
            playButton.setImage(playImage);
            playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
            if (transportSource.isPlaying()) {
                transportSource.stop();
            }
            else {
                changeState(Stopped);
            }
            timeLine.setClickableTimeStamp(true);
            break;
        }
    }
}

void MainComponent::changeLoopmode(Loopmode newLoopmode) {

        loopmode = newLoopmode;

        switch (loopmode) 
        {
        case notLooping:

            inTransition = false;
            loopButton.setImage(noLoopImage);
            timeLine.setLoopMarkersActive(false);
            timeLine.setWholeLoopMarkersActive(false);

            if(readerSource!=nullptr)
                readerSource->setLooping(false);
            
            //inf so loop or fade will not be initiated
            loopStartTime = -INFINITY;
            loopEndTime = INFINITY;
            fadeStartTime = INFINITY;
            fadeEndTime = -INFINITY;

            break;
        case loopWhole:
            
            inTransition = false;
            loopButton.setImage(wholeLoopImage);
            timeLine.setLoopMarkersActive(false);
            timeLine.setWholeLoopMarkersActive(true);

            if (readerSource != nullptr)
                readerSource->setLooping(true);

            loopStartTime = 0;
            loopEndTime = transportSource.getLengthInSeconds();
            fadeStartTime = loopEndTime-crossFade;
            fadeEndTime = loopStartTime+crossFade;
            break;
        case loopSection:

            inTransition = false;

            if (currentFile != nullptr && transportSource.getCurrentPosition() > currentFile->loopEnd) {
                changeLoopmode(fakeLoopSection);
                break;
            }

            loopButton.setImage(sectionLoopImage);
            timeLine.setLoopMarkersActive(true);
            timeLine.setWholeLoopMarkersActive(false);
            if (readerSource != nullptr)
                readerSource->setLooping(true);
            
            if (currentFile != nullptr) {

                loopStartTime = currentFile->loopStart;
                loopEndTime = currentFile->loopEnd;
                fadeStartTime = loopEndTime - crossFade;
                fadeEndTime = loopStartTime + crossFade;
            }
            else {
                loopStartTime = -INFINITY;
                loopEndTime = INFINITY;
                fadeStartTime = INFINITY;
                fadeEndTime = -INFINITY;
            }
            
            break;

        case fakeLoopSection:
            //same visual als loopSection, but no loop
            inTransition = false;
            loopButton.setImage(sectionLoopImage);
            timeLine.setLoopMarkersActive(true);
            timeLine.setWholeLoopMarkersActive(false);

            if (readerSource != nullptr)
                readerSource->setLooping(false);

            loopStartTime = -INFINITY;
            loopEndTime = INFINITY;
            fadeStartTime = INFINITY;
            fadeEndTime = -INFINITY;
            break;
        }
}

double MainComponent::timeStampToNumber(juce::String s)
{
    return s.getDoubleValue();
}

juce::String MainComponent::numberToTimeStamp(double n)
{
    return juce::String(n);
}

void MainComponent::initTimeLine() {
    timeLine.setRange(0.0, transportSource.getLengthInSeconds(), 0);
    timeLine.updateLoopMarkers();
    updateTimeLine();
}

void MainComponent::updateTimeLine()
{

    if (!timeLine.mouseIsDragged) {

        if (transportSource.getLengthInSeconds() > 0 && transportSource.getTotalLength() > 0) {
            timeLine.setValue(transportSource.getNextReadPosition()/curSampleRate);
        }
        else
        {
            timeLine.setValue(0);
        }
    }
}

void MainComponent::setLoopTimeStamps(double loopStart, double loopEnd) {

    if (loopStart < 0)
        loopStart = 0;

    if (loopEnd > transportSource.getLengthInSeconds())
        loopEnd = transportSource.getLengthInSeconds();

    if (currentFile != nullptr) {
        currentFile->loopStart = loopStart;
        currentFile->loopEnd = loopEnd;
    }
    timeLine.setLoopMarkerOnValues(loopStart, loopEnd, false);

    double curTime = transportSource.getCurrentPosition();
    if (loopmode == loopSection && curTime > loopEnd)
        changeLoopmode(fakeLoopSection);

    if (loopmode == fakeLoopSection && curTime < loopEnd)
        changeLoopmode(loopSection);

    if (loopmode == loopSection && currentFile != nullptr) {

        loopStartTime = currentFile->loopStart;
        loopEndTime = currentFile->loopEnd;

        fadeStartTime = loopEndTime - crossFade;
        fadeEndTime = loopStartTime + crossFade;
    }
}


void MainComponent::browserRootChanged(const juce::File& newRoot)
{
    bool isLibRoot = false;
    for (juce::File libRoot : musicLibs) {
        if (newRoot == libRoot) {
            isLibRoot = true;
            break;
        }
    }

    if (isLibRoot) {
        musicLibRootButton.setImage(musicLibHighlightedImage);
    }
    else {
        musicLibRootButton.setImage(musicLibImage);
    }
}

AudioFile* MainComponent::findFileInAllFiles(const juce::File& file) {
    juce::String relPath = "";
    juce::String absPath = file.getFullPathName();
    double length = transportSource.getLengthInSeconds();

    // 1. find same relative path from a musicLibRoot
    for (juce::File libRoot : musicLibs) {
        if (file.isAChildOf(libRoot)) {
            relPath = file.getRelativePathFrom(libRoot);

            for (auto it = allFiles.begin(); it != allFiles.end(); it++) {
                if (relPath == it->relPathToLib) {
                    it->length = length;
                    return &*it;
                }
            }
        }
    }

    // 2. find same absolute path
    for (auto it = allFiles.begin();it!=allFiles.end(); it++) {
        if (absPath == it->absPath) {
            it->length = transportSource.getLengthInSeconds();
            it->relPathToLib = it->relPathToLib=="" ? relPath : it->relPathToLib;
            return &*it;
        }
    }

    //if nothing found -> new file
    AudioFile newFile(absPath, 0, length);
    newFile.relPathToLib = relPath;
    newFile.length = length;

    // last: find same filename and same length and ASK if same loopmarkers should be applied
    juce::String fileName = file.getFileName();
    juce::String newLine = juce::String(juce::newLine.getDefault());
    for (auto it = allFiles.begin(); it != allFiles.end(); it++) {
        if (fileName == juce::File(it->absPath).getFileName() 
            && length == it->length) {
            int answer = juce::AlertWindow::showYesNoCancelBox(juce::MessageBoxIconType::QuestionIcon, "possible Loopmarker found!",
                juce::String("Found Loopmarker for a File with same name and length originally located here:") + newLine +
                it->absPath + newLine + newLine +
                "Should these Loopmarker positions be used for this file?" + newLine + newLine +
                "Use Same: same Loopmarkers, changes will be saved to file above" + newLine +
                "Copy: same Loopmarkers, changes will only affect this file" + newLine +
                "Neither: loopmarkers as if never opened before",
                "Use Same", "Copy", "Neither", nullptr, nullptr);
            switch (answer) {
            case 1:
                return &*it;
                break;
            case 2:
                newFile.loopStart = it->loopStart;
                newFile.loopEnd = it->loopEnd;
                break;
            default:
                break;
            }


        }
    }

    allFiles.push_back(newFile);
    return &allFiles.back();

}

void MainComponent::setCrossFade(double time)
{
    if (time > maxCrossFade)
        time = maxCrossFade;
    crossFade = time;
    fadeStartTime = loopEndTime - crossFade;
    fadeEndTime = loopStartTime + crossFade;

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


            currentFile =  findFileInAllFiles(file);

            initTimeLine();
            //changeState(state);
            changeLoopmode(loopmode);

            setLoopTimeStamps(currentFile->loopStart, currentFile->loopEnd);



        }
    }

}


void MainComponent::saveAllSettingsToFile() {
    juce::File here = juce::File::getSpecialLocation(juce::File::SpecialLocationType::currentExecutableFile).getParentDirectory();
    juce::File settingsFile = here.getChildFile("settings.json");
    settingsFile.create();

    juce::DynamicObject* obj = new juce::DynamicObject();
    juce::var json(obj);

    obj->setProperty("volume", curVolume);
    obj->setProperty("currentFileBrowserPath",fileBrowser.getRoot().getFullPathName());

    currentFile = nullptr;

    juce::var roots;
    for (juce::File file : musicLibs) {
        juce::DynamicObject* fileObj = new juce::DynamicObject();
        fileObj->setProperty("path", file.getFullPathName());
        roots.append(juce::var(fileObj));
    }
    obj->setProperty("musicLibs", roots);

    juce::var files;
    for(AudioFile file : allFiles) {
        juce::DynamicObject* fileObj = new juce::DynamicObject();
        file.copyPropetiesToDynObj(fileObj);
        files.append(juce::var(fileObj));
    }
    obj->setProperty("audioFiles", files);


    settingsFile.replaceWithText(juce::JSON::toString(json));
}

void MainComponent::loadAllSettingsFromFile() {
    juce::File here = juce::File::getSpecialLocation(juce::File::SpecialLocationType::currentExecutableFile).getParentDirectory();
    juce::File settingsFile = here.getChildFile("settings.json");
    settingsFile.create();
    juce::FileInputStream in(settingsFile);
    juce::var input = juce::JSON::parse(in);

    juce::DynamicObject* obj = input.getDynamicObject();
    if (obj != nullptr) {
        juce::var prop;

        prop = obj->getProperty("volume");
        if (prop != juce::var()) {
            curVolume = (double)prop;
            volSlider.setValue(curVolume);
        }

    

        prop = obj->getProperty("musicLibs");
        if (prop != juce::var()) {
            for (juce::var var : *prop.getArray()) {
                juce::File file = juce::File(var.getDynamicObject()->getProperty("path"));

                musicLibs.push_back(file);
            }
        }

        prop = obj->getProperty("currentFileBrowserPath");
        if (prop != juce::var()) {
            juce::File loadedRoot = juce::File(prop);
            fileBrowser.setRoot(loadedRoot);

            bool found = false;
            for (auto libRoot : musicLibs) {
                if (libRoot == loadedRoot) {
                    found = true;
                    break;
                }
            }
            musicLibRootButton.setImage(found ? musicLibHighlightedImage : musicLibImage);
        }

        prop = obj->getProperty("audioFiles");
        if (prop != juce::var()) {
            for (juce::var var : *prop.getArray()) {
                AudioFile file = AudioFile::fromVar(var);

                allFiles.push_back(file);
            }
        }

    }

    updateMusicLibsComboBox();
}

void ControlButton::setImage(juce::Image& image)
{
    normalImage = image;
    resized();
}

void ControlButton::resized()
{
    juce::Image scaledImage = normalImage.rescaled(getWidth(), getHeight(), juce::Graphics::ResamplingQuality::highResamplingQuality);

    setImages(false, true, true,
        scaledImage, 1, juce::Colours::transparentWhite,
        scaledImage, 1, juce::Colours::white.withAlpha(0.1f),
        scaledImage, 1, juce::Colours::white.withAlpha(0.3f));


}
