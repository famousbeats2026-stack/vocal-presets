#include "PluginEditor.h"

namespace
{
    // Layout constants (shared by paint() and resized()).
    constexpr int kMargin = 12;
    constexpr int kSectionsTop = 294; // below title / presets / EQ curve
    constexpr int kSectionRowH = 217;
    constexpr int kSectionColW = 219;

    juce::Rectangle<int> sectionCell(int col, int row)
    {
        return { kMargin + col * kSectionColW, kSectionsTop + row * kSectionRowH,
                 kSectionColW - 8, kSectionRowH - 8 };
    }

    const juce::Colour panelFill(0xff141210);
    const juce::Colour panelEdge(0xff6b5a26);
}

//==============================================================================
// GoldLookAndFeel
const juce::Colour GoldLookAndFeel::gold(0xffd4af37);
const juce::Colour GoldLookAndFeel::goldLight(0xfff6d47c);
const juce::Colour GoldLookAndFeel::goldDark(0xff9a7b1e);
const juce::Colour GoldLookAndFeel::black(0xff0a0a0a);
const juce::Colour GoldLookAndFeel::panel(0xff141414);
const juce::Colour GoldLookAndFeel::cream(0xfff5e6c8);

GoldLookAndFeel::GoldLookAndFeel()
{
    setColour(juce::Slider::rotarySliderFillColourId, gold);
    setColour(juce::Slider::rotarySliderOutlineColourId, goldDark);
    setColour(juce::Slider::thumbColourId, gold);
    setColour(juce::Slider::textBoxTextColourId, cream);
    setColour(juce::Slider::textBoxBackgroundColourId, panel);
    setColour(juce::Slider::textBoxOutlineColourId, goldDark);
    setColour(juce::ComboBox::backgroundColourId, panel);
    setColour(juce::ComboBox::textColourId, cream);
    setColour(juce::ComboBox::arrowColourId, gold);
    setColour(juce::ComboBox::outlineColourId, goldDark);
    setColour(juce::PopupMenu::backgroundColourId, panel);
    setColour(juce::PopupMenu::textColourId, cream);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, gold);
    setColour(juce::PopupMenu::highlightedTextColourId, black);
    setColour(juce::TextButton::buttonColourId, panel);
    setColour(juce::TextButton::buttonOnColourId, gold);
    setColour(juce::TextButton::textColourOffId, gold);
    setColour(juce::TextButton::textColourOnId, black);
}

juce::ColourGradient GoldLookAndFeel::goldGradient(float x0, float y0, float x1, float y1)
{
    juce::ColourGradient g(goldLight, x0, y0, goldDark, x1, y1, false);
    g.addColour(0.5, gold);
    return g;
}

juce::ColourGradient GoldLookAndFeel::goldRadial(float cx, float cy, float radius)
{
    juce::ColourGradient g(goldLight, cx - radius * 0.4f, cy - radius * 0.5f,
                           goldDark, cx + radius * 0.5f, cy + radius * 0.6f, true);
    g.addColour(0.55, gold);
    return g;
}

void GoldLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float rotaryStartAngle,
                                       float rotaryEndAngle, juce::Slider&)
{
    const float radius = juce::jmin(width / 2.0f, height / 2.0f) - 4.0f;
    const float centreX = x + width * 0.5f;
    const float centreY = y + height * 0.5f;
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float rw = radius * 2.0f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Background arc.
    juce::Path bgArc;
    bgArc.addArc(rx, ry, rw, rw, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(goldDark.withAlpha(0.4f));
    g.strokePath(bgArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));

    // Gold value arc.
    juce::Path valueArc;
    valueArc.addArc(rx, ry, rw, rw, rotaryStartAngle, angle, true);
    g.setColour(gold);
    g.strokePath(valueArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    // Metallic knob body.
    const float bodyR = radius - 7.0f;
    g.setGradientFill(goldRadial(centreX, centreY, bodyR));
    g.fillEllipse(centreX - bodyR, centreY - bodyR, bodyR * 2.0f, bodyR * 2.0f);

    // Gloss highlight.
    g.setColour(juce::Colours::white.withAlpha(0.38f));
    g.fillEllipse(centreX - bodyR * 0.55f, centreY - bodyR * 0.78f,
                  bodyR * 0.7f, bodyR * 0.45f);

    // Pointer line.
    juce::Path pointer;
    const float pointerLength = bodyR * 0.8f;
    pointer.addRectangle(-1.5f, -bodyR, 3.0f, pointerLength);
    g.setColour(black);
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(centreX, centreY));

    // Rim.
    g.setColour(goldDark);
    g.drawEllipse(centreX - bodyR, centreY - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);
}

void GoldLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                           const juce::Colour& /*backgroundColour*/,
                                           bool isMouseOverButton, bool isButtonDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    const float corner = 6.0f;
    const bool isOn = button.getToggleState();

    if (isOn)
    {
        // Shiny gold.
        g.setGradientFill(goldGradient(bounds.getX(), bounds.getY(),
                                       bounds.getX(), bounds.getBottom()));
        g.fillRoundedRectangle(bounds, corner);
        g.setColour(juce::Colours::white.withAlpha(0.4f));
        g.fillRoundedRectangle(bounds.getX(), bounds.getY(),
                               bounds.getWidth(), bounds.getHeight() * 0.45f, corner);
    }
    else
    {
        juce::ColourGradient grad(juce::Colour(0xff1c1a14), bounds.getX(), bounds.getY(),
                                  black, bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(bounds, corner);
        g.setColour(isMouseOverButton ? gold : goldDark);
        g.drawRoundedRectangle(bounds, corner, isMouseOverButton ? 2.0f : 1.0f);
    }

    if (isButtonDown)
    {
        g.setColour(juce::Colours::black.withAlpha(0.25f));
        g.fillRoundedRectangle(bounds, corner);
    }
}

void GoldLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                   int buttonX, int buttonY, int buttonW, int buttonH,
                                   juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();
    g.setColour(panel);
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(box.hasKeyboardFocus(true) ? gold : goldDark);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    juce::Path arrow;
    const float ax = buttonX + buttonW * 0.5f;
    const float ay = buttonY + buttonH * 0.5f;
    arrow.addTriangle(ax - 5.0f, ay - 2.0f, ax + 5.0f, ay - 2.0f, ax, ay + 4.0f);
    g.setColour(gold);
    g.fillPath(arrow);
}

//==============================================================================
// EqCurveComponent
const EqCurveComponent::Band EqCurveComponent::bands[4] = {
    { "HP",       FamousBeatsVocalPresetsProcessor::kHpFreqParam,   nullptr,
      40.0f, 200.0f, 0.0f, 0.0f },
    { "MUD",      FamousBeatsVocalPresetsProcessor::kMudFreqParam,  FamousBeatsVocalPresetsProcessor::kMudGainParam,
      150.0f, 600.0f, -12.0f, 6.0f },
    { "PRESENCE", FamousBeatsVocalPresetsProcessor::kPresFreqParam, FamousBeatsVocalPresetsProcessor::kPresGainParam,
      2000.0f, 6000.0f, -12.0f, 12.0f },
    { "AIR",      FamousBeatsVocalPresetsProcessor::kAirFreqParam,  FamousBeatsVocalPresetsProcessor::kAirGainParam,
      8000.0f, 14000.0f, -12.0f, 12.0f },
};

EqCurveComponent::EqCurveComponent(FamousBeatsVocalPresetsProcessor& p)
    : processor(p)
{
    startTimerHz(15); // live curve: presets and automation move it too
}

float EqCurveComponent::freqToX(float freq) const
{
    // Plot area: x from 34 to width-10 (leaves room for dB labels).
    const double l = std::log10(juce::jlimit(20.0f, 20000.0f, freq));
    const double l0 = std::log10(20.0), l1 = std::log10(20000.0);
    const double t = (l - l0) / (l1 - l0);
    return 34.0f + (float)t * ((float)getWidth() - 44.0f);
}

float EqCurveComponent::xToFreq(float x) const
{
    const double t = juce::jlimit(0.0, 1.0, ((double)x - 34.0) / ((double)getWidth() - 44.0));
    return (float)(20.0 * std::pow(1000.0, t));
}

float EqCurveComponent::dbToY(float db) const
{
    // Plot area: y from 8 to height-20 (leaves room for frequency labels).
    const float t = juce::jlimit(kMinDb, kMaxDb, db);
    return 8.0f + (1.0f - (t - kMinDb) / (kMaxDb - kMinDb)) * ((float)getHeight() - 28.0f);
}

float EqCurveComponent::yToDb(float y) const
{
    const float t = juce::jlimit(0.0f, 1.0f, (y - 8.0f) / ((float)getHeight() - 28.0f));
    return kMaxDb - t * (kMaxDb - kMinDb);
}

float EqCurveComponent::getParam(const char* id) const
{
    if (auto* param = dynamic_cast<juce::AudioParameterFloat*>(
            processor.getParametersState().getParameter(id)))
        return param->get();
    return 0.0f;
}

void EqCurveComponent::setParam(const char* id, float value)
{
    if (auto* param = processor.getParametersState().getParameter(id))
        param->setValueNotifyingHost(param->convertTo0to1(value));
}

juce::Point<float> EqCurveComponent::nodePosition(int band) const
{
    const auto& b = bands[band];
    const float freq = getParam(b.freqId);
    float db = 0.0f;
    if (b.gainId != nullptr)
        db = getParam(b.gainId);
    else
        db = processor.getEqResponseDb(freq); // HP node sits on the curve
    return { freqToX(freq), dbToY(db) };
}

void EqCurveComponent::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    juce::ColourGradient bg(juce::Colour(0xff171310), area.getX(), area.getY(),
                            juce::Colour(0xff050505), area.getX(), area.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(area, 6.0f);
    g.setColour(panelEdge);
    g.drawRoundedRectangle(area, 6.0f, 1.0f);

    const float left = 34.0f, right = (float)getWidth() - 10.0f;
    const float top = 8.0f, bottom = (float)getHeight() - 20.0f;

    // Grid: log frequency lines + dB lines.
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    const float freqMarks[] = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    for (float f : freqMarks)
    {
        const float x = freqToX(f);
        g.setColour(juce::Colour(0xff2a2517));
        g.drawVerticalLine((int)x, top, bottom);
        g.setColour(GoldLookAndFeel::cream.withAlpha(0.55f));
        juce::String label = f >= 1000 ? juce::String(f / 1000.0f, 0) + "k" : juce::String((int)f);
        g.drawText(label, (int)x - 14, (int)bottom + 2, 28, 12, juce::Justification::centred);
    }
    const float dbMarks[] = { -12, -6, 0, 6, 12 };
    for (float dbv : dbMarks)
    {
        const float y = dbToY(dbv);
        g.setColour(std::abs(dbv) < 0.01f ? GoldLookAndFeel::gold.withAlpha(0.7f)
                                : juce::Colour(0xff2a2517));
        g.drawHorizontalLine((int)y, left, right);
        g.setColour(GoldLookAndFeel::cream.withAlpha(0.55f));
        g.drawText((dbv > 0 ? "+" : "") + juce::String((int)dbv), 2, (int)y - 6, 28, 12,
                   juce::Justification::centredLeft);
    }

    // Response curve (computed from the real filter coefficients).
    float freqs[kNumPoints], dbs[kNumPoints];
    for (int i = 0; i < kNumPoints; ++i)
        freqs[i] = 20.0f * (float)std::pow(1000.0, (double)i / (kNumPoints - 1));
    processor.getEqResponseCurve(freqs, dbs, kNumPoints);

    auto point = [&](int i) -> juce::Point<float>
    {
        return { freqToX(freqs[i]), dbToY(dbs[i]) };
    };

    const float zeroY = dbToY(0.0f);
    juce::Path fillPath;
    fillPath.startNewSubPath(point(0));
    for (int i = 1; i < kNumPoints; ++i)
        fillPath.lineTo(point(i));
    fillPath.lineTo(point(kNumPoints - 1).x, zeroY);
    fillPath.lineTo(point(0).x, zeroY);
    fillPath.closeSubPath();
    g.setColour(GoldLookAndFeel::gold.withAlpha(0.14f));
    g.fillPath(fillPath);

    juce::Path curvePath;
    curvePath.startNewSubPath(point(0));
    for (int i = 1; i < kNumPoints; ++i)
        curvePath.lineTo(point(i));
    g.setColour(GoldLookAndFeel::gold.withAlpha(0.35f));
    g.strokePath(curvePath, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
    g.setColour(GoldLookAndFeel::goldLight);
    g.strokePath(curvePath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

    // Band nodes.
    for (int b = 0; b < 4; ++b)
    {
        const auto pos = nodePosition(b);
        const float nx = pos.x;
        const float ny = pos.y;
        const bool active = (b == draggingBand);
        g.setColour(active ? juce::Colours::white : GoldLookAndFeel::gold);
        g.fillEllipse(nx - 7, ny - 7, 14, 14);
        g.setColour(GoldLookAndFeel::black);
        g.drawEllipse(nx - 7, ny - 7, 14, 14, 2.0f);
        g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
        g.setColour(GoldLookAndFeel::cream);
        g.drawText(bands[b].name, (int)nx - 30, (int)ny - 22, 60, 12,
                   juce::Justification::centred);
    }

    // Drag readout / hint.
    g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    g.setColour(GoldLookAndFeel::gold);
    juce::String hint = dragText.isNotEmpty()
        ? dragText
        : "EQ CURVE  —  DRAG A NODE: LEFT/RIGHT = FREQUENCY, UP/DOWN = GAIN";
    g.drawText(hint, (int)left, (int)top + 2, (int)(right - left), 14,
               juce::Justification::centredLeft);
}

void EqCurveComponent::mouseDown(const juce::MouseEvent& e)
{
    int best = -1;
    float bestDist = 18.0f;
    for (int b = 0; b < 4; ++b)
    {
        const auto pos = nodePosition(b);
        const float d = std::hypot((float)e.x - pos.x, (float)e.y - pos.y);
        if (d < bestDist)
        {
            bestDist = d;
            best = b;
        }
    }
    draggingBand = best;
    if (draggingBand >= 0)
        mouseDrag(e);
}

void EqCurveComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (draggingBand < 0 || draggingBand > 3)
        return;

    const auto& b = bands[draggingBand];
    const float freq = juce::jlimit(b.minFreq, b.maxFreq, xToFreq((float)e.x));
    setParam(b.freqId, freq);

    juce::String text = juce::String(b.name) + "  " + juce::String(freq, 0) + " Hz";
    if (b.gainId != nullptr)
    {
        const float gain = juce::jlimit(b.minGain, b.maxGain, yToDb((float)e.y));
        setParam(b.gainId, gain);
        text += "  " + juce::String(gain, 1) + " dB";
    }
    dragText = text;
    repaint();
}

//==============================================================================
// PluginEditor
PluginEditor::PluginEditor(FamousBeatsVocalPresetsProcessor& p)
    : AudioProcessorEditor(&p), processor(p), eqCurve(p)
{
    setLookAndFeel(&goldLaf);
    setSize(1120, 740);

    titleLabel.setText("FAMOUS BEATS VOCAL PRESETS", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(28.0f, juce::Font::bold)));
    titleLabel.setColour(juce::Label::textColourId, GoldLookAndFeel::gold);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    brandLabel.setText("FAMOUS BEATS STUDIO  •  ONE-CLICK VOCAL CHAIN",
                       juce::dontSendNotification);
    brandLabel.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    brandLabel.setColour(juce::Label::textColourId, GoldLookAndFeel::cream);
    brandLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(brandLabel);

    makePresetButton(drakeButton, "DRAKE", 0);
    makePresetButton(travisButton, "TRAVIS SCOTT", 1);
    makePresetButton(donToliverButton, "DON TOLIVER", 2);
    makePresetButton(cleanButton, "CLEAN / RADIO", 3);
    makePresetButton(rapSingingButton, "RAP SINGING", 4);
    cleanButton.setToggleState(true, juce::dontSendNotification);

    bypassButton.setButtonText("BYPASS");
    bypassButton.setClickingTogglesState(true);
    addAndMakeVisible(bypassButton);

    presetLabel.setText("Preset: Clean / Radio", juce::dontSendNotification);
    presetLabel.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    presetLabel.setColour(juce::Label::textColourId, GoldLookAndFeel::gold);
    presetLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(presetLabel);

    addAndMakeVisible(eqCurve);

    makeSectionLabel(preampSection, "PREAMP  •  AVALON-STYLE");
    makeSectionLabel(cleanerSection, "CLEANER");
    makeSectionLabel(eqSection, "EQ");
    makeSectionLabel(compSection, "COMPRESSOR");
    makeSectionLabel(satSection, "SATURATION");
    makeSectionLabel(tuneSection, "TUNE");
    makeSectionLabel(reverbSection, "REVERB");
    makeSectionLabel(delaySection, "DELAY");
    makeSectionLabel(masterSection, "MASTER");
    makeSectionLabel(tunerSection, "TUNER");

    makeKnob(preGainKnob, preGainLabel, "GAIN", FamousBeatsVocalPresetsProcessor::kPreampGainParam);
    makeKnob(preDriveKnob, preDriveLabel, "DRIVE", FamousBeatsVocalPresetsProcessor::kPreampDriveParam);

    airButton.setButtonText("AIR");
    airButton.setClickingTogglesState(true);
    airButton.getProperties().set("fontSize", 15.0f);
    addAndMakeVisible(airButton);

    makeKnob(cleanKnob, cleanLabel, "AMOUNT", FamousBeatsVocalPresetsProcessor::kCleanAmountParam);
    makeKnob(eqKnob, eqLabel, "AMOUNT", FamousBeatsVocalPresetsProcessor::kEqAmountParam);

    makeKnob(compThreshKnob, compThreshLabel, "THRESHOLD", FamousBeatsVocalPresetsProcessor::kCompThreshParam, 48);
    makeKnob(compRatioKnob, compRatioLabel, "RATIO", FamousBeatsVocalPresetsProcessor::kCompRatioParam, 48);
    makeKnob(compAttackKnob, compAttackLabel, "ATTACK", FamousBeatsVocalPresetsProcessor::kCompAttackParam, 48);
    makeKnob(compReleaseKnob, compReleaseLabel, "RELEASE", FamousBeatsVocalPresetsProcessor::kCompReleaseParam, 48);
    makeKnob(compMixKnob, compMixLabel, "MIX", FamousBeatsVocalPresetsProcessor::kCompMixParam, 48);

    makeKnob(satKnob, satLabel, "DRIVE", FamousBeatsVocalPresetsProcessor::kSatDriveParam);
    makeKnob(satMixKnob, satMixLabel, "MIX", FamousBeatsVocalPresetsProcessor::kSatMixParam);

    makeKnob(tuneKnob, tuneLabel, "AMOUNT", FamousBeatsVocalPresetsProcessor::kTuneAmountParam);
    makeKnob(tuneSpeedKnob, tuneSpeedLabel, "SPEED", FamousBeatsVocalPresetsProcessor::kTuneSpeedParam);

    keyLabel.setText("KEY", juce::dontSendNotification);
    keyLabel.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    keyLabel.setColour(juce::Label::textColourId, GoldLookAndFeel::cream);
    keyLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(keyLabel);

    scaleLabel.setText("SCALE", juce::dontSendNotification);
    scaleLabel.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    scaleLabel.setColour(juce::Label::textColourId, GoldLookAndFeel::cream);
    scaleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(scaleLabel);

    for (int i = 0; i < 12; ++i)
        keyBox.addItem(FamousBeatsVocalPresetsProcessor::kKeyNames[i], i + 1);
    for (int i = 0; i < 5; ++i)
        scaleBox.addItem(FamousBeatsVocalPresetsProcessor::kScaleNames[i], i + 1);
    addAndMakeVisible(keyBox);
    addAndMakeVisible(scaleBox);

    makeCombo(reverbTypeBox, reverbTypeLabel, "TYPE",
              FamousBeatsVocalPresetsProcessor::kReverbTypeParam);
    for (int i = 0; i < 4; ++i)
        reverbTypeBox.addItem(FamousBeatsVocalPresetsProcessor::kReverbTypeNames[i], i + 1);

    makeKnob(verbKnob, verbLabel, "MIX", FamousBeatsVocalPresetsProcessor::kReverbMixParam);
    makeKnob(verbSizeKnob, verbSizeLabel, "SIZE", FamousBeatsVocalPresetsProcessor::kReverbSizeParam);

    delaySyncButton.setButtonText("SYNC");
    delaySyncButton.setClickingTogglesState(true);
    delaySyncButton.getProperties().set("fontSize", 12.0f);
    addAndMakeVisible(delaySyncButton);

    makeCombo(delayDivisionBox, delayDivisionLabel, "DIVISION",
              FamousBeatsVocalPresetsProcessor::kDelayDivisionParam);
    for (int i = 0; i < 7; ++i)
        delayDivisionBox.addItem(FamousBeatsVocalPresetsProcessor::kDelayDivisionNames[i], i + 1);

    makeKnob(delayBpmKnob, delayBpmLabel, "BPM", FamousBeatsVocalPresetsProcessor::kDelayBpmParam, 48);
    makeKnob(delayTimeKnob, delayTimeLabel, "TIME ms", FamousBeatsVocalPresetsProcessor::kDelayTimeParam, 48);
    makeKnob(delayFbKnob, delayFbLabel, "FEEDBACK", FamousBeatsVocalPresetsProcessor::kDelayFeedbackParam, 48);
    makeKnob(delayMixKnob, delayMixLabel, "MIX", FamousBeatsVocalPresetsProcessor::kDelayMixParam, 48);

    delayMsLabel.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    delayMsLabel.setColour(juce::Label::textColourId, GoldLookAndFeel::gold);
    delayMsLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(delayMsLabel);

    makeKnob(inputKnob, inputLabel, "INPUT", FamousBeatsVocalPresetsProcessor::kInputGainParam);
    makeKnob(outputKnob, outputLabel, "OUTPUT", FamousBeatsVocalPresetsProcessor::kOutputGainParam);

    pitchValueLabel.setFont(juce::Font(juce::FontOptions(26.0f, juce::Font::bold)));
    pitchValueLabel.setColour(juce::Label::textColourId, GoldLookAndFeel::gold);
    pitchValueLabel.setJustificationType(juce::Justification::centred);
    pitchValueLabel.setText("-- Hz", juce::dontSendNotification);
    addAndMakeVisible(pitchValueLabel);

    pitchNoteLabel.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    pitchNoteLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    pitchNoteLabel.setJustificationType(juce::Justification::centred);
    pitchNoteLabel.setText("NO PITCH DETECTED", juce::dontSendNotification);
    addAndMakeVisible(pitchNoteLabel);

    auto& params = processor.getParametersState();
    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        params, FamousBeatsVocalPresetsProcessor::kTuneKeyParam, keyBox));
    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        params, FamousBeatsVocalPresetsProcessor::kTuneScaleParam, scaleBox));
    // NOTE: reverbTypeBox and delayDivisionBox already got their
    // attachments from makeCombo() above — do not attach them twice.
    buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        params, FamousBeatsVocalPresetsProcessor::kBypassParam, bypassButton));
    buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        params, FamousBeatsVocalPresetsProcessor::kPreampAirParam, airButton));
    buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        params, FamousBeatsVocalPresetsProcessor::kDelaySyncParam, delaySyncButton));

    startTimerHz(10); // tuner readout + delay ms label
}

void PluginEditor::makeKnob(juce::Slider& knob, juce::Label& label, const juce::String& text,
                            const char* paramId, int /*knobSize*/)
{
    knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 18);
    knob.setColour(juce::Slider::rotarySliderFillColourId, GoldLookAndFeel::gold);
    knob.setColour(juce::Slider::rotarySliderOutlineColourId, GoldLookAndFeel::goldDark);
    knob.setColour(juce::Slider::thumbColourId, GoldLookAndFeel::gold);
    knob.setColour(juce::Slider::textBoxTextColourId, GoldLookAndFeel::cream);
    knob.setColour(juce::Slider::textBoxBackgroundColourId, GoldLookAndFeel::panel);
    addAndMakeVisible(knob);

    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);

    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getParametersState(), paramId, knob));
}

void PluginEditor::makeCombo(juce::ComboBox& box, juce::Label& label, const juce::String& text,
                             const char* paramId)
{
    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);

    box.setColour(juce::ComboBox::backgroundColourId, GoldLookAndFeel::panel);
    box.setColour(juce::ComboBox::textColourId, GoldLookAndFeel::cream);
    box.setColour(juce::ComboBox::arrowColourId, GoldLookAndFeel::gold);
    box.setColour(juce::ComboBox::outlineColourId, GoldLookAndFeel::goldDark);
    addAndMakeVisible(box);

    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.getParametersState(), paramId, box));
}

void PluginEditor::makeSectionLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    label.setColour(juce::Label::textColourId, GoldLookAndFeel::gold);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
}

void PluginEditor::makePresetButton(juce::TextButton& button, const juce::String& text, int presetIndex)
{
    button.setButtonText(text);
    button.setClickingTogglesState(false);
    button.getProperties().set("fontSize", 13.0f);
    addAndMakeVisible(button);
    button.onClick = [this, presetIndex] { selectPreset(presetIndex); };
}

void PluginEditor::selectPreset(int presetIndex)
{
    processor.applyPreset(presetIndex);
    drakeButton.setToggleState(presetIndex == 0, juce::dontSendNotification);
    travisButton.setToggleState(presetIndex == 1, juce::dontSendNotification);
    donToliverButton.setToggleState(presetIndex == 2, juce::dontSendNotification);
    cleanButton.setToggleState(presetIndex == 3, juce::dontSendNotification);
    rapSingingButton.setToggleState(presetIndex == 4, juce::dontSendNotification);
    presetLabel.setText("Preset: " + juce::String(FamousBeatsVocalPresetsProcessor::getPresetName(presetIndex)),
                        juce::dontSendNotification);
}

void PluginEditor::timerCallback()
{
    // Delay time readout.
    const double delaySecs = processor.getDelayTimeSeconds();
    double bpm = processor.getHostBpm();
    if (bpm <= 0.0)
    {
        if (auto* param = dynamic_cast<juce::AudioParameterFloat*>(
                processor.getParametersState().getParameter(
                    FamousBeatsVocalPresetsProcessor::kDelayBpmParam)))
            bpm = param->get();
    }
    delayMsLabel.setText("DELAY  " + juce::String(delaySecs * 1000.0, 0) + " ms  @  "
                         + juce::String(bpm, 0) + " BPM", juce::dontSendNotification);

    // Tuner readout.
    const float f = processor.getDetectedPitchHz();
    if (f > 0.0f)
    {
        const double midi = 69.0 + 12.0 * std::log2(f / 440.0);
        const int m = (int)std::lrint(midi);
        const int pc = ((m % 12) + 12) % 12;
        const int octave = m / 12 - 1;
        const double target = 440.0 * std::pow(2.0, (m - 69) / 12.0);
        const double cents = 1200.0 * std::log2(f / target);
        pitchValueLabel.setText(juce::String(f, 1) + " Hz", juce::dontSendNotification);
        pitchNoteLabel.setText(juce::String(FamousBeatsVocalPresetsProcessor::kKeyNames[pc])
                               + juce::String(octave)
                               + (cents >= 0 ? "  +" : "  ")
                               + juce::String((int)std::lrint(cents)) + " cents",
                               juce::dontSendNotification);
    }
    else
    {
        pitchValueLabel.setText("-- Hz", juce::dontSendNotification);
        pitchNoteLabel.setText("NO PITCH DETECTED", juce::dontSendNotification);
    }
}

void PluginEditor::paint(juce::Graphics& g)
{
    // Shiny black background: vertical gradient with a gold top edge.
    juce::ColourGradient bg(juce::Colour(0xff171208), 0.0f, 0.0f,
                            juce::Colour(0xff050505), 0.0f, (float)getHeight(), false);
    g.setGradientFill(bg);
    g.fillAll();
    g.setColour(GoldLookAndFeel::gold);
    g.fillRect(0, 0, getWidth(), 2);
    g.setColour(GoldLookAndFeel::goldDark);
    g.drawRect(getLocalBounds(), 2);

    // Section panels: dark fill, gold edge, gold underline beneath the title.
    for (int row = 0; row < 2; ++row)
    {
        for (int col = 0; col < 5; ++col)
        {
            auto cell = sectionCell(col, row).toFloat();
            g.setColour(panelFill);
            g.fillRoundedRectangle(cell, 6.0f);
            g.setColour(panelEdge);
            g.drawRoundedRectangle(cell, 6.0f, 1.0f);
            g.setColour(GoldLookAndFeel::gold.withAlpha(0.5f));
            g.drawLine(cell.getX() + 10, cell.getY() + 24, cell.getRight() - 10,
                       cell.getY() + 24, 1.0f);
        }
    }
}

void PluginEditor::resized()
{
    auto area = getLocalBounds().reduced(kMargin);
    titleLabel.setBounds(area.removeFromTop(34));
    brandLabel.setBounds(area.removeFromTop(16));
    area.removeFromTop(4);

    // Preset row: five preset buttons + bypass.
    auto presetRow = area.removeFromTop(42);
    const int gaps = 5 * 8; // one 8px gap after each preset button
    const int btnW = (presetRow.getWidth() - gaps - 100) / 5;
    auto placeButton = [&](juce::TextButton& b)
    {
        b.setBounds(presetRow.removeFromLeft(btnW));
        presetRow.removeFromLeft(8);
    };
    placeButton(drakeButton);
    placeButton(travisButton);
    placeButton(donToliverButton);
    placeButton(cleanButton);
    placeButton(rapSingingButton);
    bypassButton.setBounds(presetRow.removeFromLeft(100));
    area.removeFromTop(4);
    presetLabel.setBounds(area.removeFromTop(18));
    area.removeFromTop(6);

    // EQ curve strip.
    eqCurve.setBounds(area.removeFromTop(150));
    area.removeFromTop(8);

    // Sections: two rows of five panels.
    // Row 1.
    {
        auto cell = sectionCell(0, 0);
        preampSection.setBounds(cell.removeFromTop(24));
        auto knobRow = cell.removeFromTop(96);
        const int half = knobRow.getWidth() / 2;
        preGainKnob.setBounds(juce::Rectangle<int>(knobRow.getX(), knobRow.getY(), half, 78).withSizeKeepingCentre(78, 78));
        preGainLabel.setBounds(juce::Rectangle<int>(knobRow.getX(), knobRow.getY() + 78, half, 18));
        preDriveKnob.setBounds(juce::Rectangle<int>(knobRow.getX() + half, knobRow.getY(), half, 78).withSizeKeepingCentre(78, 78));
        preDriveLabel.setBounds(juce::Rectangle<int>(knobRow.getX() + half, knobRow.getY() + 78, half, 18));
        cell.removeFromTop(4);
        airButton.setBounds(cell.removeFromTop(34).reduced(8, 0));
    }
    {
        auto cell = sectionCell(1, 0);
        cleanerSection.setBounds(cell.removeFromTop(24));
        auto knobRow = cell.removeFromTop(96);
        cleanKnob.setBounds(knobRow.withSizeKeepingCentre(96, 96));
        cleanLabel.setBounds(juce::Rectangle<int>(cell.getX(), knobRow.getY() + 96, cell.getWidth(), 18));
    }
    {
        auto cell = sectionCell(2, 0);
        eqSection.setBounds(cell.removeFromTop(24));
        auto knobRow = cell.removeFromTop(96);
        eqKnob.setBounds(knobRow.withSizeKeepingCentre(96, 96));
        eqLabel.setBounds(juce::Rectangle<int>(cell.getX(), knobRow.getY() + 96, cell.getWidth(), 18));
    }
    {
        auto cell = sectionCell(3, 0);
        compSection.setBounds(cell.removeFromTop(24));
        const int smallW = cell.getWidth() / 3;
        auto placeSmall = [&](juce::Slider& k, juce::Label& l, int x, int y)
        {
            k.setBounds(juce::Rectangle<int>(x, y, smallW, 62).withSizeKeepingCentre(62, 62));
            l.setBounds(juce::Rectangle<int>(x, y + 62, smallW, 16));
        };
        placeSmall(compThreshKnob, compThreshLabel, cell.getX(), cell.getY());
        placeSmall(compRatioKnob, compRatioLabel, cell.getX() + smallW, cell.getY());
        placeSmall(compAttackKnob, compAttackLabel, cell.getX() + 2 * smallW, cell.getY());
        placeSmall(compReleaseKnob, compReleaseLabel, cell.getX() + smallW / 2, cell.getY() + 78);
        placeSmall(compMixKnob, compMixLabel, cell.getX() + smallW + smallW / 2, cell.getY() + 78);
    }
    {
        auto cell = sectionCell(4, 0);
        satSection.setBounds(cell.removeFromTop(24));
        auto knobRow = cell.removeFromTop(96);
        const int half = knobRow.getWidth() / 2;
        satKnob.setBounds(juce::Rectangle<int>(knobRow.getX(), knobRow.getY(), half, 78).withSizeKeepingCentre(78, 78));
        satLabel.setBounds(juce::Rectangle<int>(knobRow.getX(), knobRow.getY() + 78, half, 18));
        satMixKnob.setBounds(juce::Rectangle<int>(knobRow.getX() + half, knobRow.getY(), half, 78).withSizeKeepingCentre(78, 78));
        satMixLabel.setBounds(juce::Rectangle<int>(knobRow.getX() + half, knobRow.getY() + 78, half, 18));
    }

    // Row 2.
    {
        auto cell = sectionCell(0, 1);
        tuneSection.setBounds(cell.removeFromTop(24));
        auto knobRow = cell.removeFromTop(96);
        const int half = knobRow.getWidth() / 2;
        tuneKnob.setBounds(juce::Rectangle<int>(knobRow.getX(), knobRow.getY(), half, 78).withSizeKeepingCentre(78, 78));
        tuneLabel.setBounds(juce::Rectangle<int>(knobRow.getX(), knobRow.getY() + 78, half, 18));
        tuneSpeedKnob.setBounds(juce::Rectangle<int>(knobRow.getX() + half, knobRow.getY(), half, 78).withSizeKeepingCentre(78, 78));
        tuneSpeedLabel.setBounds(juce::Rectangle<int>(knobRow.getX() + half, knobRow.getY() + 78, half, 18));
        auto keyArea = cell.removeFromTop(14);
        keyLabel.setBounds(keyArea.removeFromLeft(cell.getWidth() / 2));
        scaleLabel.setBounds(keyArea);
        auto boxRow = cell.removeFromTop(28);
        keyBox.setBounds(boxRow.removeFromLeft(cell.getWidth() / 2).reduced(4, 2));
        scaleBox.setBounds(boxRow.reduced(4, 2));
    }
    {
        auto cell = sectionCell(1, 1);
        reverbSection.setBounds(cell.removeFromTop(24));
        reverbTypeLabel.setBounds(cell.removeFromTop(14));
        reverbTypeBox.setBounds(cell.removeFromTop(28).reduced(6, 2));
        auto knobRow = cell.removeFromTop(96);
        const int half = knobRow.getWidth() / 2;
        verbKnob.setBounds(juce::Rectangle<int>(knobRow.getX(), knobRow.getY(), half, 78).withSizeKeepingCentre(78, 78));
        verbLabel.setBounds(juce::Rectangle<int>(knobRow.getX(), knobRow.getY() + 78, half, 18));
        verbSizeKnob.setBounds(juce::Rectangle<int>(knobRow.getX() + half, knobRow.getY(), half, 78).withSizeKeepingCentre(78, 78));
        verbSizeLabel.setBounds(juce::Rectangle<int>(knobRow.getX() + half, knobRow.getY() + 78, half, 18));
    }
    {
        auto cell = sectionCell(2, 1);
        delaySection.setBounds(cell.removeFromTop(24));
        auto topRow = cell.removeFromTop(28);
        delaySyncButton.setBounds(topRow.removeFromLeft(64).reduced(2, 2));
        delayDivisionLabel.setBounds(topRow.removeFromLeft(64));
        delayDivisionBox.setBounds(topRow.reduced(2, 2));
        delayMsLabel.setBounds(cell.removeFromTop(16));
        const int smallW = cell.getWidth() / 4;
        auto placeSmall = [&](juce::Slider& k, juce::Label& l, int x, int y)
        {
            k.setBounds(juce::Rectangle<int>(x, y, smallW, 62).withSizeKeepingCentre(62, 62));
            l.setBounds(juce::Rectangle<int>(x, y + 62, smallW, 16));
        };
        placeSmall(delayBpmKnob, delayBpmLabel, cell.getX(), cell.getY());
        placeSmall(delayTimeKnob, delayTimeLabel, cell.getX() + smallW, cell.getY());
        placeSmall(delayFbKnob, delayFbLabel, cell.getX() + 2 * smallW, cell.getY());
        placeSmall(delayMixKnob, delayMixLabel, cell.getX() + 3 * smallW, cell.getY());
    }
    {
        auto cell = sectionCell(3, 1);
        masterSection.setBounds(cell.removeFromTop(24));
        auto knobRow = cell.removeFromTop(96);
        const int half = knobRow.getWidth() / 2;
        inputKnob.setBounds(juce::Rectangle<int>(knobRow.getX(), knobRow.getY(), half, 78).withSizeKeepingCentre(78, 78));
        inputLabel.setBounds(juce::Rectangle<int>(knobRow.getX(), knobRow.getY() + 78, half, 18));
        outputKnob.setBounds(juce::Rectangle<int>(knobRow.getX() + half, knobRow.getY(), half, 78).withSizeKeepingCentre(78, 78));
        outputLabel.setBounds(juce::Rectangle<int>(knobRow.getX() + half, knobRow.getY() + 78, half, 18));
    }
    {
        auto cell = sectionCell(4, 1);
        tunerSection.setBounds(cell.removeFromTop(24));
        pitchValueLabel.setBounds(cell.removeFromTop(44));
        pitchNoteLabel.setBounds(cell.removeFromTop(24));
    }
}
