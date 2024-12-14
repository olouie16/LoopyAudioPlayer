#pragma once

#include <JuceHeader.h>


class TimeLine;

class LoopMarker : public juce::Component
{
public:

    LoopMarker(TimeLine* par);

    enum colourIds {
        baseColor = 0x30000010,
        outlineColour
    };

    void paint(juce::Graphics& g) override;


    void resized() override;

    void setActive(bool active);


    //anchor is at x: center, y: top 
    void setTimeStamp(double val);
    double getTimeStamp() {return timeStamp;}
    void setPosition(float x, float y);

    void update();

    TimeLine* par;
    //juce::TextEditor inputBox;
    juce::Image activeIcon;
    juce::Image inactiveIcon;

private:

    double timeStamp = 0;
    double height = 10;
    double width = 8;
    juce::Point<double> drawAtPoint;
    juce::Rectangle<int> bounds;

    bool active;

    juce::AffineTransform transform;

    void createIcons();
};

class TimeLine : public juce::Slider,
    public juce::Timer
{
public:
    TimeLine();
    ~TimeLine() {};
    
    std::function<void()> onTimerCallback;
    std::function<void(bool)> onValueChange;
    std::function<void(double, double)> onLoopMarkerChange;

    double guiRefreshTime = 100; //ms;
    bool mouseIsDragged = false;

    void paint(juce::Graphics& g);
    void resized();

    void updateLoopMarkers();
    double getValueFromPosition(float pos);
    double getValueFromPosition(const juce::MouseEvent& e);

    void setLoopMarkerOnValue(double val);
    void setLoopMarkerOnValues(double val1, double val2, bool callback);
    double getLeftLoopTimestamp(){return leftMarker.getTimeStamp();}
    double getRightLoopTimestamp(){return rightMarker.getTimeStamp();}

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;


    void setLoopMarkersActive(bool active);
    void setWholeLoopMarkersActive(bool active);
    juce::Image getActiveLoopMarkerIcon();

private:
    void timerCallback() final { onTimerCallback(); };
    LoopMarker leftMarker = LoopMarker(this);
    LoopMarker rightMarker = LoopMarker(this);

    bool wholeLoopActive;
};





