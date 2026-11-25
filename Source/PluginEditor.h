#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <memory>

class DC23LookAndFeel final : public juce::LookAndFeel_V4 {
public:
    DC23LookAndFeel();
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float,
                          float, float, juce::Slider&) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool, bool) override;
};

class DC23AudioProcessorEditor final : public juce::AudioProcessorEditor {
public:
    explicit DC23AudioProcessorEditor(DC23AudioProcessor&);
    ~DC23AudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct Module {
        juce::Slider amount, character, flux;
        juce::ToggleButton enabled;
        std::unique_ptr<SliderAttachment> amountLink, characterLink, fluxLink;
        std::unique_ptr<ButtonAttachment> enabledLink;
    };

    void setupKnob(juce::Slider&, const juce::String& id,
                   std::unique_ptr<SliderAttachment>& attachment);
    void openPreset(bool saving);
    static juce::Rectangle<int> moduleBounds(int index);

    DC23AudioProcessor& processor;
    DC23LookAndFeel look;
    std::array<Module, 6> modules;
    juce::Slider magnitude, mix, output, lowCut, highCut, width;
    juce::Slider echoMix, echoTime, echoFeedback, digitalSmooth;
    std::array<std::unique_ptr<SliderAttachment>, 10> masterLinks;
    juce::ComboBox factoryPresets, noiseStyle, distortionStyle;
    std::unique_ptr<ComboAttachment> noiseStyleLink, distortionStyleLink;
    juce::TextButton saveButton {"SAVE"}, loadButton {"LOAD"};
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DC23AudioProcessorEditor)
};
