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

    void setToActiveColours();
    void setToInactiveColours();

    //anchor is at x: center, y: top 
    void setPosition(float x, float y);

    void setTimeStamp(double val);

    double getTimeStamp() {return timeStamp;}

    void update();

    TimeLine* par;
    //juce::TextEditor inputBox;

private:

    int x1=0;
    int y1=0;
    int x2=0;
    int y2=0;
    int x3=0;
    int y3=0;

    double timeStamp = 0;
    double height = 10;
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

    void mouseDown(const juce::MouseEvent& e);
    void mouseUp(const juce::MouseEvent& e);
    void mouseDrag(const juce::MouseEvent& e);

    void hideLoopMarkers();
    void showLoopMarkers();

private:
    void timerCallback() final { onTimerCallback(); };
    LoopMarker leftMarker = LoopMarker(this);
    LoopMarker rightMarker = LoopMarker(this);

};





