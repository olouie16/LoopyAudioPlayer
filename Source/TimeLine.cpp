#include "TimeLine.h"
#include <regex>
#include <format>

LoopMarker::LoopMarker(TimeLine* par) :par(par) {

    setBounds(0, 0, 1, 1);
    //setInterceptsMouseClicks(true, false);
    createIcons();
}

void LoopMarker::paint(juce::Graphics& g)
{
    g.drawImageAt(active ? activeIcon : inactiveIcon, drawAtPoint.x, drawAtPoint.y);
}

void LoopMarker::resized() {
    setPosition(par->getPositionOfValue(timeStamp), par->getHeight()/2.0 - height - 6);
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

    drawAtPoint = juce::Point<float>(x - width / 2, y);
    bounds = juce::Rectangle<int>(floor(x - width / 2),
                                floor(y),
                                width+1,
                                height+1);
    setBounds(bounds);
    repaint();

}

void LoopMarker::mouseDown(const juce::MouseEvent& e)
{
    //juce::MouseEvent newE = e.withNewPosition(juce::Point<float>(par->getPositionOfValue(timeStamp) + e.x - width/2, par->getHeight() / 2 - height - 6));
    par->loopMarkerClick(timeStamp, false, false);
}

void LoopMarker::mouseUp(const juce::MouseEvent& e)
{
    //juce::MouseEvent newE = e.withNewPosition(juce::Point<float>(par->getPositionOfValue(timeStamp) + e.x - width / 2, par->getHeight() / 2 - height - 6));
    //par->loopMarkerClick(timeStamp, false, false);
}

void LoopMarker::mouseDrag(const juce::MouseEvent& e)
{
    juce::MouseEvent newE = e.withNewPosition(juce::Point<float>(par->getPositionOfValue(timeStamp) + e.x - width / 2, par->getHeight() / 2 - height - 6));
    par->loopMarkerClick(par->getValueFromPosition(newE), true, false);
}

void LoopMarker::mouseDoubleClick(const juce::MouseEvent& e)
{
    //juce::MouseEvent newE = e.withNewPosition(juce::Point<float>(par->getPositionOfValue(timeStamp) + e.x - width / 2, par->getHeight() / 2 - height - 6));
    par->loopMarkerClick(timeStamp, false, true);
}

void LoopMarker::setTimeStamp(double val) {
    timeStamp = val;
    resized();
}

void LoopMarker::update() {
    setPosition(par->getPositionOfValue(timeStamp), getPosition().y);
}



TimeLine::TimeLine() {

    textFromValueFunction = [this](double value) {

        bool milliSecsPrecision = showMilliSeconds;
        int hours = floor(value / 3600);
        value -= hours * 3600;
        int minutes = floor(value / 60);
        value -= minutes * 60;
        int seconds = ((int)floor(value));
        value -= seconds;
        int milliseconds = (int)round(value * 1000);

        return  (hours > 0 ? juce::String(hours) + ":" : "") +
            (hours > 0 && minutes < 10 ? "0" : "") + juce::String(minutes) + ":" +
            (seconds < 10 ? "0" : "") + juce::String(seconds) +
            (milliSecsPrecision ? std::format(".{:0>3}", milliseconds) : "");
    };

    valueFromTextFunction = [this](const juce::String& text) {
        const std::regex rgx(R"(^(?:([0-9]+):)?([0-9]+):([0-9]+)(?:,|.([0-9]+))?$)");
        std::smatch m;
        std::string stdString = text.toStdString();
        std::regex_match(stdString, m, rgx);

        double val = 0;
        
        if (!m.empty()) {


            int hours = m.str(1).empty() ? 0 : std::stoi(m.str(1));
            val += hours * 3600;

            int minutes = std::stoi(m.str(2));
            val += minutes * 60;

            int seconds = std::stoi(m.str(3));
            val += seconds;

            //int milliSeconds = m.str(4).empty() ? 0 : std::stoi(m.str(4));
            for (int i = 0; i < m.str(4).length(); i++) {
                val += std::stoi(m.str(4).substr(i, 1)) / std::pow(10, i + 1);
            }

        }
        else {
            //no match
            val = -1;
        }

        return val;
    };

    onDragStart = [this]() {
        mouseIsDragged = true;
    };

    onDragEnd = [this]() {
        if (onValueChange)
            onValueChange(true);
        mouseIsDragged = false;
    };

    inputBoxFadeFunction = [this] {

        if (inputBoxFadeTimer.getTimerInterval() > 50) {
            inputBoxFadeTimer.startTimer(50);
        }
        else {
            inputBox.setAlpha(inputBox.getAlpha() - 0.05);
            if (inputBox.getAlpha() <= 0)
            {
                inputBox.setVisible(false);
                showMilliSeconds = false;
                inputBoxFadeTimer.stopTimer();
            }
        }
    };

    inputBox.onTextChange = [this] {
        float width = inputBox.getFont().getStringWidthFloat(inputBox.getText());
        width += inputBox.getLeftIndent() * 2;
        inputBox.setBounds(round(inputBoxCenterX - width / 2.0), inputBox.getPosition().y, width, inputBox.getHeight());
    };
    
    inputBox.onReturnKey = [this] {
        inputBox.giveAwayKeyboardFocus();
        double val = getValueFromText(inputBox.getText());
        if (val == -1) {
            val = lastLoopMarkerPos;
        }

        double maxVal = getMaximum();
        val = val <= maxVal ? val : maxVal;

        if (leftMarker.getTimeStamp() == lastLoopMarkerPos) {
            setLoopMarkerOnValues(val, rightMarker.getTimeStamp());
        }
        else {
            setLoopMarkerOnValues(leftMarker.getTimeStamp(), val);
        }

        positionInputBox(val);

        inputBox.setReadOnly(true);
        inputBoxFadeTimer.startTimer(1000);
    };

    inputBox.onEscapeKey = [this] {
        inputBox.giveAwayKeyboardFocus();
        inputBox.setText(getTextFromValue(lastLoopMarkerPos));
    };

    inputBox.onFocusLost = [this] {
        inputBoxFadeTimer.startTimer(1000);
        inputBox.setReadOnly(true);
    };



    setTextBoxIsEditable(true);

    timeStampBox = findTimeStampBox();
    if (timeStampBox!=nullptr) {
        setClickableTimeStamp(true);
    }

    addAndMakeVisible(leftMarker);
    addAndMakeVisible(rightMarker);

    inputBox.setMultiLine(false);
    inputBox.setJustification(juce::Justification::centred);
    addAndMakeVisible(inputBox);
    inputBox.setVisible(false);

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
    
    juce::Slider::resized();

    positionInputBox();

    leftMarker.resized();
    rightMarker.resized();

}

void TimeLine::positionInputBox(double time) {

    double value = time >= 0 ? time : getValue();
    inputBox.setText(getTextFromValue(value));

    float inputBoxWidth = inputBox.getFont().getStringWidthFloat(inputBox.getText()) + inputBox.getLeftIndent() * 2;
    float inputBoxHeight = 16;

    inputBoxCenterX = getPositionOfValue(value);

    inputBox.setBounds(getPositionOfValue(value) - 0.5 * inputBoxWidth, 0.62 * getHeight(), inputBoxWidth, 16);

}

void TimeLine::startShowingInputBox(double time) {

    showMilliSeconds = true;
    positionInputBox(time);
    inputBox.setVisible(true);
    inputBox.setAlpha(1);
    inputBox.setReadOnly(time<0);
    inputBox.setCaretVisible(false);
    inputBoxFadeTimer.startTimer(1000);
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
void TimeLine::setLoopMarkerOnValues(double val1, double val2, bool callback) {

    if (val1 > val2) {
        std::swap(val1, val2);
    }

    val1 = val1 >= 0 ? val1 : 0;
    val2 = val2 >= 0 ? val2 : 0;
    double maxVal = getMaximum();
    val1 = val1 <= maxVal ? val1 : maxVal;
    val2 = val2 <= maxVal ? val2 : maxVal;

    leftMarker.setTimeStamp(val1);
    rightMarker.setTimeStamp(val2);

    if (callback && onLoopMarkerChange)
        onLoopMarkerChange(leftMarker.getTimeStamp(), rightMarker.getTimeStamp());
}

void TimeLine::loopMarkerClick(double atTimeValue, bool isNewPos, bool isDoubleClick) {


    if(isNewPos)
        setLoopMarkerOnValue(atTimeValue);

    startShowingInputBox(atTimeValue);

    if (isDoubleClick) {
        lastLoopMarkerPos = atTimeValue;
        inputBox.setReadOnly(false);
        inputBox.setCaretVisible(true);
        inputBoxFadeTimer.stopTimer();
        inputBox.grabKeyboardFocus();
    }
}

void TimeLine::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isShiftDown()) {
        //setting loopMarkers
        loopMarkerClick(getValueFromPosition(e), true, false);
    }
    else {
        //normal slider behavior
        juce::Slider::mouseDown(e);
        startShowingInputBox();
    }

}

void TimeLine::mouseUp(const juce::MouseEvent& e) {
    if (e.mods.isShiftDown()) {
        //setting loopMarkers
        loopMarkerClick(getValueFromPosition(e), true, false);
    }
    else {
        //normal slider behavior
        juce::Slider::mouseUp(e);
        startShowingInputBox();
    }
}

void TimeLine::mouseDrag(const juce::MouseEvent& e) {

    if (e.mods.isShiftDown()) {
        //setting loopMarkers
        loopMarkerClick(getValueFromPosition(e), true, false);
    }
    else {
        //normal slider behavior
        juce::Slider::mouseDrag(e);
        startShowingInputBox();
    }

    inputBox.setReadOnly(true);
    inputBox.setCaretVisible(false);

}

void TimeLine::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (e.mods.isShiftDown()) {
        //setting loopMarkers
        loopMarkerClick(getValueFromPosition(e), true, true);

    }
    else {
        //normal slider behavior
        juce::Slider::mouseDoubleClick(e);
    }

}

void TimeLine::childrenChanged()
{
    timeStampBox = findTimeStampBox();

    setClickableTimeStamp(true);

    timeStampBox->onEditorShow = [this] {
        showMilliSeconds = true;
        timeStampBox = findTimeStampBox();
        if (timeStampBox != nullptr) {
            juce::TextEditor* te = timeStampBox->getCurrentTextEditor();
            te->setText(getTextFromValue(getValue()), juce::NotificationType::sendNotificationAsync);
            te->setJustification(juce::Justification::centred);
            te->moveCaretToEnd();
            te->selectAll();
        }

    };
    timeStampBox->onEditorHide = [this] {showMilliSeconds = false; };
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

const juce::Image TimeLine::getActiveLoopMarkerIcon()
{
    return leftMarker.activeIcon;
}

juce::Label* TimeLine::findTimeStampBox()
{
    for (auto* child : getChildren())
    {
        if (auto* l = dynamic_cast<juce::Label*> (child))
            return l;
    }

    return nullptr;
}

void TimeLine::setClickableTimeStamp(bool shouldBeClickable)
{
    if (shouldBeClickable) {
        timeStampBox->setEditable(false, true, true);
    }
    else {
        timeStampBox->setEditable(false, false, true);

    }
}





