#include "TimeLine.h"



LoopMarker::LoopMarker(TimeLine* par) :par(par) {
    setColour(baseColor, juce::Colours::orange);
    setColour(outlineColour, juce::Colours::darkorange);

    //inputBox.setMultiLine(false);
    //par->addChildComponent(inputBox);
}

void LoopMarker::paint(juce::Graphics& g)
{

    g.setColour(findColour(baseColor));

    juce::Path triangle;
    triangle.addTriangle(x1, y1, x2, y2, x3, y3);
    g.fillPath(triangle);
    g.setColour(findColour(outlineColour));
    g.strokePath(triangle, juce::PathStrokeType::PathStrokeType(2, juce::PathStrokeType::JointStyle::mitered));
}

void LoopMarker::resized() {
    setPosition(par->getPositionOfValue(timeStamp), 5);
}

void LoopMarker::setToActiveColours() {
    setColour(baseColor, juce::Colours::orange);
    setColour(outlineColour, juce::Colours::darkorange);
}

void LoopMarker::setToInactiveColours() {
    setColour(baseColor, juce::Colours::orange.darker(0.7));
    setColour(outlineColour, juce::Colours::darkorange.darker(0.7));
}

//anchor is at x: center, y: top 
void LoopMarker::setPosition(float x, float y) {
    x1 = x - height / 2;
    x2 = x + height / 2;
    x3 = x;
    y1 = y;
    y2 = y;
    y3 = y + height;
    repaint();

    //inputBox.setBounds(x-20, par->getHeight()/2+4, 40, 16);
    //inputBox.setVisible(true);
}

void LoopMarker::setTimeStamp(double val) {
    timeStamp = val;
    setPosition(par->getPositionOfValue(timeStamp), getPosition().y + (height / 2));
}

void LoopMarker::update() {
    setPosition(par->getPositionOfValue(timeStamp), getPosition().y);
}



TimeLine::TimeLine() {

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
        if (onValueChange)
            onValueChange(true);
        mouseIsDragged = false;
        };

    addAndMakeVisible(leftMarker);
    addAndMakeVisible(rightMarker);


    leftMarker.setTimeStamp(0);
    rightMarker.setTimeStamp(INFINITY);
};

void TimeLine::paint(juce::Graphics& g)
{
    juce::Slider::paint(g);
    leftMarker.paint(g);
    rightMarker.paint(g);

}

void TimeLine::resized()
{
    juce::Slider::resized();
    leftMarker.resized();
    rightMarker.resized();

}

void TimeLine::updateLoopMarkers() {
    leftMarker.update();
    rightMarker.update();
}

/// <summary>
/// Returns the value of this TimeLine if clicked at this position. If this TimeLine is horizontal the x coordinate should be given, if vertical the y coordinate.
/// </summary>
/// <param name="pos">position on relevant axis</param>
/// <returns>a value on this TimeLine</returns>
double TimeLine::getValueFromPosition(float pos) {

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
double TimeLine::getValueFromPosition(const juce::MouseEvent& e) {

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
void TimeLine::setLoopMarkerOnValue(double val) {

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
/// <param name="val2">second new value for loop markers</param>
/// <param name="callback">whether onLoopMarkerChange should be called or not</param>
void TimeLine::setLoopMarkerOnValues(double val1, double val2, bool callback = true) {

    if (val1 < val2) {
        leftMarker.setTimeStamp(val1);
        rightMarker.setTimeStamp(val2);
    }
    else {
        leftMarker.setTimeStamp(val2);
        rightMarker.setTimeStamp(val1);
    }

    if (callback && onLoopMarkerChange)
        onLoopMarkerChange(leftMarker.getTimeStamp(), rightMarker.getTimeStamp());
}

void TimeLine::mouseDown(const juce::MouseEvent& e)
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

void TimeLine::mouseUp(const juce::MouseEvent& e) {
    if (e.mods.isShiftDown()) {
        //setting loopMarkers
        double time = getValueFromPosition(e);
        setLoopMarkerOnValue(time);
    }
    else {
        //normal slider behavior
        juce::Slider::mouseUp(e);
    }
}

void TimeLine::mouseDrag(const juce::MouseEvent& e) {

    if (e.mods.isShiftDown()) {
        //setting loopMarkers
        double time = getValueFromPosition(e);
        setLoopMarkerOnValue(time);
    }
    else {
        //normal slider behavior
        juce::Slider::mouseDrag(e);
    }
}

void TimeLine::hideLoopMarkers() {

    leftMarker.setToInactiveColours();
    rightMarker.setToInactiveColours();
    repaint();

}

void TimeLine::showLoopMarkers() {

    leftMarker.setToActiveColours();
    rightMarker.setToActiveColours();
    repaint();

}




