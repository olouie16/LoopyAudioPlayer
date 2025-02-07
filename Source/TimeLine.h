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

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

    void update();

    TimeLine* par;
    juce::Image activeIcon;
    juce::Image inactiveIcon;

private:

    double timeStamp = 0;
    double height = 10;
    double width = 8;
    juce::Point<float> drawAtPoint;
    juce::Rectangle<int> bounds;

    bool active;

    //juce::AffineTransform transform;

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

    double guiRefreshTime = 50; //ms;
    bool mouseIsDragged = false;

    void paint(juce::Graphics& g);
    void resized();


    void positionInputBox(double time=-1);
    void startShowingInputBox(double time=-1);
    double lastLoopMarkerPos=0;


    void updateLoopMarkers();
    double getValueFromPosition(float pos);
    double getValueFromPosition(const juce::MouseEvent& e);

    void setLoopMarkerOnValue(double val);
    void setLoopMarkerOnValues(double val1, double val2, bool callback=true);
    double getLeftLoopTimestamp(){return leftMarker.getTimeStamp();}
    double getRightLoopTimestamp(){return rightMarker.getTimeStamp();}

    void loopMarkerClick(double atTimeValue, bool isNewPos, bool isDoubleClick);
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

    void childrenChanged() override;
    void setClickableTimeStamp(bool shouldBeClickable);

    void setLoopMarkersActive(bool active);
    void setWholeLoopMarkersActive(bool active);
    const juce::Image getActiveLoopMarkerIcon();

private:
    void timerCallback() final { onTimerCallback(); };
    LoopMarker leftMarker = LoopMarker(this);
    LoopMarker rightMarker = LoopMarker(this);

    bool wholeLoopActive = false;
    juce::Label* timeStampBox;
    juce::Label* findTimeStampBox();

    bool showMilliSeconds=false;
    juce::TextEditor inputBox;
    std::function<void()> inputBoxFadeFunction;
    juce::TimedCallback inputBoxFadeTimer{ [this] {inputBoxFadeFunction(); } };

    float inputBoxCenterX = 0;
};





