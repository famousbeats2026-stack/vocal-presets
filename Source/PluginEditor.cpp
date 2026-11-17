#include "PluginEditor.h"

namespace
{
    const juce::Colour gold(0xffd4af37);
    const juce::Colour goldDark(0xff9a7b1e);
    const juce::Colour black(0xff0a0a0a);
    const juce::Colour panel(0xff141414);
    const juce::Colour panelEdge(0xff2a2415);
    const juce::Colour cream(0xfff5e6c8);
}

PluginEditor::PluginEditor(FamousBeatsVocalPresetsProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(880, 540);

    titleLabel.setText("FAMOUS BEATS VOCAL PRESETS", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(28.0f, juce::Font::bold)));
    titleLabel.setColour(juce::Label::textColourId, gold);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    brandLabel.setText("FAMOUS BEATS STUDIO  •  ONE-CLICK VOCAL CHAIN",
                       juce::dontSendNotification);
    brandLabel.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    brandLabel.setColour(juce::Label::textColourId, cream);
    brandLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(brandLabel);

    auto stylePresetButton = [](juce::TextButton& b, const juce::String& text)
    {
        b.setButtonText(text);
        b.setColour(juce::TextButton::buttonColourId, panel);
        b.setColour(juce::TextButton::buttonOnColourId, gold);
        b.setColour(juce::TextButton::textColourOffId, gold);
        b.setColour(juce::TextButton::textColourOnId, black);
        b.setColour(juce::TextButton::buttonColourId, panel);
    };
    stylePresetButton(drakeButton, "DRAKE");
    stylePresetButton(travisButton, "TRAVIS SCOTT");
    stylePresetButton(donToliverButton, "DON TOLIVER");
    stylePresetButton(cleanButton, "CLEAN / RADIO");
    addAndMakeVisible(drakeButton);
    addAndMakeVisible(travisButton);
    addAndMakeVisible(donToliverButton);
    addAndMakeVisible(cleanButton);

    drakeButton.onClick = [this] { selectPreset(0); };
    travisButton.onClick = [this] { selectPreset(1); };
    donToliverButton.onClick = [this] { selectPreset(2); };
    cleanButton.onClick = [this] { selectPreset(3); };

    bypassButton.setButtonText("BYPASS");
    bypassButton.setClickingTogglesState(true);
    bypassButton.setColour(juce::TextButton::buttonColourId, panel);
    bypassButton.setColour(juce::TextButton::buttonOnColourId, goldDark);
    bypassButton.setColour(juce::TextButton::textColourOffId, cream);
    bypassButton.setColour(juce::TextButton::textColourOnId, black);
    addAndMakeVisible(bypassButton);

    presetLabel.setText("Preset: Clean / Radio", juce::dontSendNotification);
    presetLabel.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    presetLabel.setColour(juce::Label::textColourId, gold);
    presetLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(presetLabel);

    makeSectionLabel(cleanerSection, "CLEANER");
    makeSectionLabel(eqSection, "EQ");
    makeSectionLabel(compSection, "COMPRESSOR");
    makeSectionLabel(satSection, "SATURATION");
    makeSectionLabel(tuneSection, "TUNE");
    makeSectionLabel(reverbSection, "REVERB");
    makeSectionLabel(delaySection, "DELAY");
    makeSectionLabel(masterSection, "MASTER");

    makeKnob(cleanKnob, cleanLabel, "AMOUNT", FamousBeatsVocalPresetsProcessor::kCleanAmountParam);
    makeKnob(eqKnob, eqLabel, "AMOUNT", FamousBeatsVocalPresetsProcessor::kEqAmountParam);
    makeKnob(airKnob, airLabel, "AIR", FamousBeatsVocalPresetsProcessor::kAirGainParam);
    makeKnob(compThreshKnob, compThreshLabel, "THRESHOLD", FamousBeatsVocalPresetsProcessor::kCompThreshParam);
    makeKnob(compMixKnob, compMixLabel, "MIX", FamousBeatsVocalPresetsProcessor::kCompMixParam);
    makeKnob(satKnob, satLabel, "DRIVE", FamousBeatsVocalPresetsProcessor::kSatDriveParam);
    makeKnob(satMixKnob, satMixLabel, "MIX", FamousBeatsVocalPresetsProcessor::kSatMixParam);
    makeKnob(tuneKnob, tuneLabel, "AMOUNT", FamousBeatsVocalPresetsProcessor::kTuneAmountParam);
    makeKnob(tuneSpeedKnob, tuneSpeedLabel, "SPEED", FamousBeatsVocalPresetsProcessor::kTuneSpeedParam);
    makeKnob(verbKnob, verbLabel, "MIX", FamousBeatsVocalPresetsProcessor::kReverbMixParam);
    makeKnob(verbSizeKnob, verbSizeLabel, "SIZE", FamousBeatsVocalPresetsProcessor::kReverbSizeParam);
    makeKnob(delayTimeKnob, delayTimeLabel, "TIME", FamousBeatsVocalPresetsProcessor::kDelayTimeParam);
    makeKnob(delayMixKnob, delayMixLabel, "MIX", FamousBeatsVocalPresetsProcessor::kDelayMixParam);
    makeKnob(inputKnob, inputLabel, "INPUT", FamousBeatsVocalPresetsProcessor::kInputGainParam);
    makeKnob(outputKnob, outputLabel, "OUTPUT", FamousBeatsVocalPresetsProcessor::kOutputGainParam);

    keyLabel.setText("KEY", juce::dontSendNotification);
    keyLabel.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    keyLabel.setColour(juce::Label::textColourId, cream);
    keyLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(keyLabel);

    scaleLabel.setText("SCALE", juce::dontSendNotification);
    scaleLabel.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    scaleLabel.setColour(juce::Label::textColourId, cream);
    scaleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(scaleLabel);

    for (int i = 0; i < 12; ++i)
        keyBox.addItem(FamousBeatsVocalPresetsProcessor::kKeyNames[i], i + 1);
    for (int i = 0; i < 5; ++i)
        scaleBox.addItem(FamousBeatsVocalPresetsProcessor::kScaleNames[i], i + 1);
    keyBox.setColour(juce::ComboBox::backgroundColourId, panel);
    keyBox.setColour(juce::ComboBox::textColourId, cream);
    keyBox.setColour(juce::ComboBox::arrowColourId, gold);
    scaleBox.setColour(juce::ComboBox::backgroundColourId, panel);
    scaleBox.setColour(juce::ComboBox::textColourId, cream);
    scaleBox.setColour(juce::ComboBox::arrowColourId, gold);
    addAndMakeVisible(keyBox);
    addAndMakeVisible(scaleBox);

    auto& params = processor.getParametersState();
    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        params, FamousBeatsVocalPresetsProcessor::kTuneKeyParam, keyBox));
    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        params, FamousBeatsVocalPresetsProcessor::kTuneScaleParam, scaleBox));
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        params, FamousBeatsVocalPresetsProcessor::kBypassParam, bypassButton);
}

void PluginEditor::makeKnob(juce::Slider& knob, juce::Label& label, const juce::String& text,
                            const char* paramId)
{
    knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 18);
    knob.setColour(juce::Slider::rotarySliderFillColourId, gold);
    knob.setColour(juce::Slider::rotarySliderOutlineColourId, goldDark);
    knob.setColour(juce::Slider::thumbColourId, gold);
    knob.setColour(juce::Slider::textBoxTextColourId, cream);
    knob.setColour(juce::Slider::textBoxBackgroundColourId, panel);
    addAndMakeVisible(knob);

    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    label.setColour(juce::Label::textColourId, cream);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);

    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getParametersState(), paramId, knob));
}

void PluginEditor::makeSectionLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    label.setColour(juce::Label::textColourId, gold);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
}

void PluginEditor::selectPreset(int presetIndex)
{
    processor.applyPreset(presetIndex);
    presetLabel.setText("Preset: " + juce::String(FamousBeatsVocalPresetsProcessor::getPresetName(presetIndex)),
                        juce::dontSendNotification);
}

void PluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(black);
    g.setColour(goldDark);
    g.drawRect(getLocalBounds(), 2);

    // Section panel outlines.
    g.setColour(panelEdge);
    const int top = 144;
    const int rowH = 186;
    const int colW = (getWidth() - 32) / 4;
    for (int row = 0; row < 2; ++row)
        for (int col = 0; col < 4; ++col)
            g.drawRect(16 + col * colW, top + row * rowH, colW - 8, rowH - 8, 1);
}

void PluginEditor::resized()
{
    auto area = getLocalBounds().reduced(16);
    titleLabel.setBounds(area.removeFromTop(36));
    brandLabel.setBounds(area.removeFromTop(18));
    area.removeFromTop(6);

    auto presetRow = area.removeFromTop(40);
    const int btnW = 150;
    drakeButton.setBounds(presetRow.removeFromLeft(btnW));
    presetRow.removeFromLeft(8);
    travisButton.setBounds(presetRow.removeFromLeft(btnW));
    presetRow.removeFromLeft(8);
    donToliverButton.setBounds(presetRow.removeFromLeft(btnW));
    presetRow.removeFromLeft(8);
    cleanButton.setBounds(presetRow.removeFromLeft(btnW));
    presetRow.removeFromLeft(8);
    bypassButton.setBounds(presetRow.removeFromLeft(110));
    area.removeFromTop(4);
    presetLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(4);

    // Two rows of four sections.
    const int colW = area.getWidth() / 4;
    const int rowH = 186;
    auto layoutSection = [&](int col, int row, juce::Label& section,
                             std::vector<std::pair<juce::Slider*, juce::Label*>> knobs)
    {
        auto cell = juce::Rectangle<int>(16 + col * colW, 144 + row * rowH, colW - 8, rowH - 8);
        section.setBounds(cell.removeFromTop(22));
        const int knobW = cell.getWidth() / (int)knobs.size();
        for (auto& [knob, label] : knobs)
        {
            auto kArea = cell.removeFromLeft(knobW);
            knob->setBounds(kArea.removeFromTop(96));
            label->setBounds(kArea.removeFromTop(18));
        }
    };

    layoutSection(0, 0, cleanerSection, { { &cleanKnob, &cleanLabel } });
    layoutSection(1, 0, eqSection, { { &eqKnob, &eqLabel }, { &airKnob, &airLabel } });
    layoutSection(2, 0, compSection, { { &compThreshKnob, &compThreshLabel }, { &compMixKnob, &compMixLabel } });
    layoutSection(3, 0, satSection, { { &satKnob, &satLabel }, { &satMixKnob, &satMixLabel } });
    layoutSection(0, 1, tuneSection, { { &tuneKnob, &tuneLabel }, { &tuneSpeedKnob, &tuneSpeedLabel } });
    layoutSection(1, 1, reverbSection, { { &verbKnob, &verbLabel }, { &verbSizeKnob, &verbSizeLabel } });
    layoutSection(2, 1, delaySection, { { &delayTimeKnob, &delayTimeLabel }, { &delayMixKnob, &delayMixLabel } });
    layoutSection(3, 1, masterSection, { { &inputKnob, &inputLabel }, { &outputKnob, &outputLabel } });

    // Key / scale selectors sit under the Tune knobs.
    auto tuneCell = juce::Rectangle<int>(16, 144 + rowH, colW - 8, rowH - 8);
    tuneCell.removeFromTop(22 + 96 + 18); // section title + knobs + labels
    auto keyArea = tuneCell.removeFromLeft((colW - 8) / 2);
    keyLabel.setBounds(keyArea.removeFromTop(14));
    keyBox.setBounds(keyArea.removeFromTop(26).reduced(4, 0));
    scaleLabel.setBounds(tuneCell.removeFromTop(14));
    scaleBox.setBounds(tuneCell.removeFromTop(26).reduced(4, 0));
}
