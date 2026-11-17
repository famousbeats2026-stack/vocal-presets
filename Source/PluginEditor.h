#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// Black-and-gold editor: four big preset buttons up top, then one knob
// group per chain section (Cleaner / EQ / Comp / Saturation / Tune /
// Reverb / Delay / Master). Key and scale selectors live in the Tune
// section; presets set every section but never force a key.
class PluginEditor : public juce::AudioProcessorEditor
{
public:
    explicit PluginEditor(FamousBeatsVocalPresetsProcessor&);
    ~PluginEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void makeKnob(juce::Slider& knob, juce::Label& label, const juce::String& text,
                  const char* paramId);
    void makeSectionLabel(juce::Label& label, const juce::String& text);
    void selectPreset(int presetIndex);

    FamousBeatsVocalPresetsProcessor& processor;

    juce::Label titleLabel;
    juce::Label brandLabel;
    juce::Label presetLabel;

    juce::TextButton drakeButton;
    juce::TextButton travisButton;
    juce::TextButton donToliverButton;
    juce::TextButton cleanButton;
    juce::TextButton bypassButton;

    // Section labels.
    juce::Label cleanerSection, eqSection, compSection, satSection;
    juce::Label tuneSection, reverbSection, delaySection, masterSection;

    // Knobs + labels.
    juce::Slider cleanKnob;   juce::Label cleanLabel;
    juce::Slider eqKnob;      juce::Label eqLabel;
    juce::Slider airKnob;     juce::Label airLabel;
    juce::Slider compThreshKnob; juce::Label compThreshLabel;
    juce::Slider compMixKnob; juce::Label compMixLabel;
    juce::Slider satKnob;     juce::Label satLabel;
    juce::Slider satMixKnob;  juce::Label satMixLabel;
    juce::Slider tuneKnob;    juce::Label tuneLabel;
    juce::Slider tuneSpeedKnob; juce::Label tuneSpeedLabel;
    juce::Slider verbKnob;    juce::Label verbLabel;
    juce::Slider verbSizeKnob; juce::Label verbSizeLabel;
    juce::Slider delayTimeKnob; juce::Label delayTimeLabel;
    juce::Slider delayMixKnob; juce::Label delayMixLabel;
    juce::Slider inputKnob;   juce::Label inputLabel;
    juce::Slider outputKnob;  juce::Label outputLabel;

    juce::ComboBox keyBox;
    juce::ComboBox scaleBox;
    juce::Label keyLabel;
    juce::Label scaleLabel;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
