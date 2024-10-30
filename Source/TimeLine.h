#pragma once

#include <JuceHeader.h>


class TimeLine;
class LoopMarker : public juce::Component
{
public:

    LoopMarker() {
        setColour(baseColor, juce::Colours::orange);
        setColour(outlineColour, juce::Colours::darkorange);
    }

    enum colourIds {
        baseColor = 0x30000010,
        outlineColour
    };

    void paint(juce::Graphics& g) override
    {

        g.setColour(findColour(baseColor));
       
        juce::Path triangle;
        triangle.addTriangle(x1,y1,x2,y2,x3,y3);
        g.fillPath(triangle);
        g.setColour(findColour(outlineColour));
        g.strokePath(triangle, juce::PathStrokeType::PathStrokeType(2, juce::PathStrokeType::JointStyle::mitered));
    }



    //x: center, y: top
    void setPosition(float x, float y) {
        x1 = x - height/2;
        x2 = x + height/2;
        x3 = x;
        y1 = y;
        y2 = y;
        y3 = y + height;
        repaint();
    }

    void setTimeStamp(double val) {
        timeStamp = val;
        setPosition(getPosFromValue(timeStamp), getPosition().y);
    }

    double getTimeStamp() {
        return timeStamp;
    }

    void update() {
        setPosition(getPosFromValue(timeStamp), getPosition().y);
    }

    std::function<float(double)> getPosFromValue;

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
    TimeLine() {

        textFromValueFunction = [](double value) {
            int seconds = ((int)floor(value)) % 60;
            int minutes = value / 60;
            int hours = minutes / 60;
            if (hours > 0) { minutes = minutes % 60; }

            return  (hours > 0 ? juce::String(hours) + ":" : "") +
                (hours > 0 && minutes < 10 ? "0" : "") + juce::String(minutes) + ":" +
                (seconds < 10 ? "0" : "") + juce::String(seconds);
            };

        onDragStart = [this]() {
            mouseIsDragged = true;
            };

        onDragEnd = [this]() {
            //DBG(getValue());
            if(onValueChange)
                onValueChange(true);
            mouseIsDragged = false;
            };


        addAndMakeVisible(leftMarker);
        addAndMakeVisible(rightMarker);

        leftMarker.getPosFromValue = [this](double val) {return getPositionOfValue(val); };
        rightMarker.getPosFromValue = [this](double val) {return getPositionOfValue(val); };
        leftMarker.setTimeStamp(10.0);
        rightMarker.setTimeStamp(15.0);
    };


    ~TimeLine() {};
    void paint(juce::Graphics& g) override
    {
        juce::Slider::paint(g);
        leftMarker.paint(g);
        rightMarker.paint(g);

    }

    void resized() override
    {
        juce::Slider::resized();
        leftMarker.setPosition(getPositionOfValue(leftMarker.getTimeStamp()), getHeight()/4.0 - 2);
        rightMarker.setPosition(getPositionOfValue(rightMarker.getTimeStamp()), getHeight()/4.0 - 2);


    }
    
    std::function<void()> onTimerCallback;
    std::function<void(bool)> onValueChange;
    std::function<void(double, double)> onLoopMarkerChange;

    double guiRefreshTime = 100; //ms;
    bool mouseIsDragged = false;

    void updateLoopMarkers() {
        leftMarker.update();
        rightMarker.update();
    }


    /// <summary>
    /// Returns the value of this TimeLine if clicked at this position. If this TimeLine is horizontal the x coordinate should be given, if vertical the y coordinate.
    /// </summary>
    /// <param name="pos">position on relevant axis</param>
    /// <returns>a value on this TimeLine</returns>
    double getValueFromPosition(float pos) {

        int sliderRegionStart;
        int sliderRegionSize;

        juce::LookAndFeel& lf = getLookAndFeel();

        auto layout = lf.getSliderLayout(*this);


        if (isHorizontal())
        {
            sliderRegionStart = layout.sliderBounds.getX();
            sliderRegionSize = layout.sliderBounds.getWidth();
        }
        else if (isVertical())
        {
            sliderRegionStart = layout.sliderBounds.getY();
            sliderRegionSize = layout.sliderBounds.getHeight();
        }
        else {
            return 0.0;
        }

        double normVal = (pos - sliderRegionStart) / sliderRegionSize;

        if (normVal < 0) {
            return proportionOfLengthToValue(0);
        }
        else if (normVal > 1) {
            return proportionOfLengthToValue(1);
        }
        else {
            return proportionOfLengthToValue(normVal);
        }
    }


    /// <summary>
    /// Returns the value of this TimeLine if clicked at this position.
    /// </summary>
    /// <param name="e">MouseEvent containing a position</param>
    /// <returns>a value on this TimeLine</returns>
    double getValueFromPosition(const juce::MouseEvent& e) {

        if (isHorizontal())
        {
            getValueFromPosition(e.position.x);
        }
        else if (isVertical())
        {
            getValueFromPosition(e.position.y);
        }
        else {
            return 0.0;
        }
    }

    /// <summary>
    /// Sets nearest loop marker on given value.
    /// </summary>
    /// <param name="val">new value for loop marker</param>
    void setLoopMarkerOnValue(double val) {

        if (val > rightMarker.getTimeStamp()) {
            rightMarker.setTimeStamp(val);
        }
        else if (val < leftMarker.getTimeStamp()) {
            leftMarker.setTimeStamp(val);
        }
        else {
            if (val < (leftMarker.getTimeStamp() + rightMarker.getTimeStamp()) / 2) {
                leftMarker.setTimeStamp(val);
            }
            else {
                rightMarker.setTimeStamp(val);
            }
        }

        if (onLoopMarkerChange)
            onLoopMarkerChange(leftMarker.getTimeStamp(), rightMarker.getTimeStamp());
    }

    /// <summary>
    /// Sets both loop markers on given values. order of values is irrelevant.
    /// </summary>
    /// <param name="val1">first new value for loop markers</param>
    /// <param name="val2"></param>
    void setLoopMarkerOnValues(double val1, double val2) {

        if (val1 < val2) {
            leftMarker.setTimeStamp(val1);
            rightMarker.setTimeStamp(val2);
        }
        else {
            leftMarker.setTimeStamp(val2);
            rightMarker.setTimeStamp(val1);
        }

        if (onLoopMarkerChange)
            onLoopMarkerChange(leftMarker.getTimeStamp(), rightMarker.getTimeStamp());
    }

    void mouseDown(const juce::MouseEvent& e)
    {
        if (e.mods.isShiftDown()) {
            //setting loopMarkers
            double time = getValueFromPosition(e);
            setLoopMarkerOnValue(time);
        }
        else {
            //normal slider behavior
            juce::Slider::mouseDown(e);
        }

    }

    void mouseUp(const juce::MouseEvent& e) {
        if (e.mods.isShiftDown()) {
            //setting loopMarkers
        }
        else {
            //normal slider behavior
            juce::Slider::mouseUp(e);
        }
    }

    void mouseDrag(const juce::MouseEvent& e) {

        if (e.mods.isShiftDown()) {
            //setting loopMarkers

        }
        else {
            //normal slider behavior
            juce::Slider::mouseDrag(e);
        }
    }
private:
    void timerCallback() final { onTimerCallback(); };
    LoopMarker leftMarker;
    LoopMarker rightMarker;

};





