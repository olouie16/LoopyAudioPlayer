#include "TimeLine.h"



LoopMarker::LoopMarker(TimeLine* par) :par(par) {
    //par->addChildComponent(inputBox);
    

    //inputBox.setMultiLine(false);
    //setBounds(0, 0, 1, 1);
    setInterceptsMouseClicks(true, false);
    createIcons();
}

void LoopMarker::paint(juce::Graphics& g)
{
    g.drawImageAt(active ? activeIcon : inactiveIcon, drawAtPoint.x, drawAtPoint.y);
}

void LoopMarker::resized() {
    setPosition(par->getPositionOfValue(timeStamp), par->getHeight()/2 - height - 6);
}

void LoopMarker::setActive(bool active) {
    this->active = active;
}


void LoopMarker::createIcons() {
    int x1 = 0;
    int x2 = width;
    int x3 = width/2;
    int y1 = 0;
    int y2 = 0;
    int y3 = height;
    juce::Path p;
    p.addTriangle(x1, y1, x2, y2, x3, y3);

    activeIcon = juce::Image(juce::Image::PixelFormat::ARGB, width, height, true);
    juce::Graphics g(activeIcon);
    g.setColour(juce::Colours::orange);
    g.fillPath(p);
    g.setColour(juce::Colours::darkorange);
    g.strokePath(p, juce::PathStrokeType::PathStrokeType(2, juce::PathStrokeType::JointStyle::mitered));

    inactiveIcon = juce::Image(juce::Image::PixelFormat::ARGB, width, height, true);
    juce::Graphics g2(inactiveIcon);
    g2.setColour(juce::Colours::orange.darker(0.7));
    g2.fillPath(p);
    g2.setColour(juce::Colours::darkorange.darker(0.7));
    g2.strokePath(p, juce::PathStrokeType::PathStrokeType(2, juce::PathStrokeType::JointStyle::mitered));

}

//anchor is at x: center, y: top 
void LoopMarker::setPosition(float x, float y) {

    drawAtPoint = juce::Point<double>(x - width / 2, y);
    bounds = juce::Rectangle<int>(floor(x - width / 2),
                                floor(y),
                                width+1,
                                height+1);
    setBounds(bounds);
    repaint();

    //inputBox.setBounds(x-20, par->getHeight()/2+4, 40, 16);
    //inputBox.setVisible(true);
}

void LoopMarker::setTimeStamp(double val) {
    timeStamp = val;
    resized();
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

    juce::Colour fillCol = juce::Colours::blue;
    juce::Colour outlineCol = juce::Colours::black;

    if (!wholeLoopActive) {
        fillCol = fillCol.interpolatedWith(juce::Colours::darkgrey, 0.4);
        outlineCol = outlineCol.interpolatedWith(juce::Colours::darkgrey, 0.4);
    }

    auto sliderbounds = getLookAndFeel().getSliderLayout(*this).sliderBounds;
    juce::Rectangle<float> wholeMarker(getTextBoxWidth()+1, getHeight()/2-10, 5, 20);
    g.setColour(fillCol);
    g.fillRoundedRectangle(wholeMarker, 1);
    g.setColour(outlineCol);
    g.drawRoundedRectangle(wholeMarker, 1, 1);

    wholeMarker.setX(getWidth() - 6);
    //g.setColour(juce::Colours::blue);
    g.setColour(fillCol);
    g.fillRoundedRectangle(wholeMarker, 1);
    g.setColour(outlineCol);
    g.drawRoundedRectangle(wholeMarker, 1, 1);
}

void TimeLine::resized()
{
    getLookAndFeel().getSliderLayout(*this).sliderBounds.removeFromLeft(100);
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
        return getValueFromPosition(e.position.x);
    }
    else if (isVertical())
    {
        return getValueFromPosition(e.position.y);
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

void TimeLine::setLoopMarkersActive(bool active) {

    leftMarker.setActive(active);
    rightMarker.setActive(active);
    repaint();

}

void TimeLine::setWholeLoopMarkersActive(bool active)
{
    wholeLoopActive = active;
}

juce::Image TimeLine::getActiveLoopMarkerIcon()
{
    return leftMarker.activeIcon;
}





