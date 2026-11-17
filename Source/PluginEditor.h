#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Gold, shiny LookAndFeel: metallic gold gradients, gloss highlights,
// black text on gold, white text on black.
class GoldLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GoldLookAndFeel();

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override;
    juce::Font getTextButtonFont(juce::TextButton& button, int buttonHeight) override
    {
        // Per-button size via a "fontSize" property; default scales with height.
        const auto& props = button.getProperties();
        const float size = props.contains("fontSize")
            ? (float)(double)props["fontSize"]
            : juce::jmin(14.0f, buttonHeight * 0.4f);
        return juce::Font(juce::FontOptions(size, juce::Font::bold));
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool isMouseOverButton, bool isButtonDown) override;
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    static juce::ColourGradient goldGradient(float x0, float y0, float x1, float y1);
    static juce::ColourGradient goldRadial(float cx, float cy, float radius);

    static const juce::Colour gold;
    static const juce::Colour goldLight;
    static const juce::Colour goldDark;
    static const juce::Colour black;
    static const juce::Colour panel;
    static const juce::Colour cream;
};

//==============================================================================
// Draggable EQ curve display: draws the combined EQ magnitude response
// (from the processor's actual filter coefficients) on a log-frequency /
// dB grid, with a draggable node per band. Drag a node left/right to
// change its frequency, up/down to change its gain. The high-pass node
// moves in frequency only.
class EqCurveComponent : public juce::Component, private juce::Timer
{
public:
    explicit EqCurveComponent(FamousBeatsVocalPresetsProcessor& p);
    ~EqCurveComponent() override = default;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent&) override { draggingBand = -1; repaint(); }

private:
    void timerCallback() override { repaint(); }

    struct Band
    {
        const char* name;
        const char* freqId;
        const char* gainId; // nullptr for the high-pass (frequency only)
        float minFreq, maxFreq;
        float minGain, maxGain;
    };
    static const Band bands[4];

    float freqToX(float freq) const;
    float xToFreq(float x) const;
    float dbToY(float db) const;
    float yToDb(float y) const;
    juce::Point<float> nodePosition(int band) const;
    float getParam(const char* id) const;
    void setParam(const char* id, float value);

    FamousBeatsVocalPresetsProcessor& processor;
    int draggingBand = -1;
    juce::String dragText;

    static constexpr int kNumPoints = 160;
    static constexpr float kMinDb = -15.0f;
    static constexpr float kMaxDb = 15.0f;
};

//==============================================================================
// Black-and-gold editor, redesigned: shiny gold LookAndFeel, draggable EQ
// curve strip up top, then one panel per chain section (Preamp / Cleaner /
// EQ / Comp / Saturation / Tune / Reverb / Delay / Master) plus a tuner
// readout. Five big preset buttons: Drake / Travis Scott / Don Toliver /
// Clean Radio / Rap Singing.
class PluginEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PluginEditor(FamousBeatsVocalPresetsProcessor&);
    ~PluginEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    void makeKnob(juce::Slider& knob, juce::Label& label, const juce::String& text,
                  const char* paramId, int knobSize = 64);
    void makeCombo(juce::ComboBox& box, juce::Label& label, const juce::String& text,
                   const char* paramId);
    void makeSectionLabel(juce::Label& label, const juce::String& text);
    void makePresetButton(juce::TextButton& button, const juce::String& text, int presetIndex);
    void selectPreset(int presetIndex);

    FamousBeatsVocalPresetsProcessor& processor;
    GoldLookAndFeel goldLaf;

    juce::Label titleLabel;
    juce::Label brandLabel;
    juce::Label presetLabel;

    juce::TextButton drakeButton;
    juce::TextButton travisButton;
    juce::TextButton donToliverButton;
    juce::TextButton cleanButton;
    juce::TextButton rapSingingButton;
    juce::TextButton bypassButton;

    EqCurveComponent eqCurve;

    // Section labels.
    juce::Label preampSection, cleanerSection, eqSection, compSection, satSection;
    juce::Label tuneSection, reverbSection, delaySection, masterSection, tunerSection;

    // Preamp.
    juce::Slider preGainKnob;  juce::Label preGainLabel;
    juce::Slider preDriveKnob; juce::Label preDriveLabel;
    juce::TextButton airButton;

    // Cleaner / EQ.
    juce::Slider cleanKnob; juce::Label cleanLabel;
    juce::Slider eqKnob;    juce::Label eqLabel;

    // Compressor.
    juce::Slider compThreshKnob;  juce::Label compThreshLabel;
    juce::Slider compRatioKnob;   juce::Label compRatioLabel;
    juce::Slider compAttackKnob;  juce::Label compAttackLabel;
    juce::Slider compReleaseKnob; juce::Label compReleaseLabel;
    juce::Slider compMixKnob;     juce::Label compMixLabel;

    // Saturation.
    juce::Slider satKnob;    juce::Label satLabel;
    juce::Slider satMixKnob; juce::Label satMixLabel;

    // Tune.
    juce::Slider tuneKnob;      juce::Label tuneLabel;
    juce::Slider tuneSpeedKnob; juce::Label tuneSpeedLabel;
    juce::ComboBox keyBox;
    juce::ComboBox scaleBox;
    juce::Label keyLabel;
    juce::Label scaleLabel;

    // Reverb.
    juce::ComboBox reverbTypeBox;
    juce::Label reverbTypeLabel;
    juce::Slider verbKnob;     juce::Label verbLabel;
    juce::Slider verbSizeKnob; juce::Label verbSizeLabel;

    // Delay.
    juce::TextButton delaySyncButton;
    juce::ComboBox delayDivisionBox;
    juce::Label delayDivisionLabel;
    juce::Slider delayBpmKnob;   juce::Label delayBpmLabel;
    juce::Slider delayTimeKnob;  juce::Label delayTimeLabel;
    juce::Slider delayFbKnob;    juce::Label delayFbLabel;
    juce::Slider delayMixKnob;   juce::Label delayMixLabel;
    juce::Label delayMsLabel;

    // Master.
    juce::Slider inputKnob;  juce::Label inputLabel;
    juce::Slider outputKnob; juce::Label outputLabel;

    // Tuner readout.
    juce::Label pitchValueLabel;
    juce::Label pitchNoteLabel;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
