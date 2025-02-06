/*
  ==============================================================================

    OwnLookAndFeel.h
    Created: 23 Jan 2025 6:19:09pm
    Author:  ottoh

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class OwnLookAndFeel : public juce::LookAndFeel_V4
{
public:
    OwnLookAndFeel() {};
    ~OwnLookAndFeel() {};

    juce::Button* createFileBrowserMusicLibButton()
    {
        auto* musicLibButton = new juce::DrawableButton("musicLib", juce::DrawableButton::ImageOnButtonBackground);

        musicLibButton->setToggleable(true);
        musicLibButton->setClickingTogglesState(false);

        juce::Path musicLibShape = createMusicLibShape();


        juce::DrawablePath musicLibImage;
        musicLibImage.setFill(findColour(juce::TextButton::textColourOffId));
        musicLibImage.setPath(musicLibShape);

        juce::DrawablePath musicLibOnImage;
        musicLibOnImage.setFill(findColour(juce::TextButton::textColourOnId));
        //musicLibOnImage.setFill(juce::Colours::green);
        musicLibOnImage.setPath(musicLibShape);

        musicLibButton->setImages(&musicLibImage, nullptr, nullptr, nullptr, &musicLibOnImage);

        return musicLibButton;
    
    };

private:
    juce::Path createMusicLibShape() {

        float imageSize = 50;
        float topDiff = 0.15 * imageSize;
        float topBottomGab = 0.1 * imageSize;

        juce::Path musicLibShape;
        musicLibShape.startNewSubPath(0.5 * imageSize, topDiff + topBottomGab);
        musicLibShape.lineTo(imageSize, topDiff + topBottomGab);
        musicLibShape.lineTo(imageSize, imageSize - topBottomGab);
        musicLibShape.lineTo(0, imageSize - topBottomGab);
        musicLibShape.lineTo(0, topBottomGab);
        musicLibShape.lineTo(0.3 * imageSize, topBottomGab);
        musicLibShape = musicLibShape.createPathWithRoundedCorners(0.1 * imageSize);
        musicLibShape.closeSubPath();

        return musicLibShape;
    };


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OwnLookAndFeel)
};








