#pragma once
#include <JuceHeader.h>
#include <array>
#include <memory>
#include "PluginProcessor.h"

class DioVerbLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    DioVerbLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float,
                          float, float, juce::Slider&) override;
};

class DioVerbEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit DioVerbEditor(DioVerbProcessor&);
    ~DioVerbEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct DragPad final : juce::Component {
        std::function<juce::File()> fetchFile;
        void paint(juce::Graphics&) override;
        void mouseDown(const juce::MouseEvent&) override;
        void mouseDrag(const juce::MouseEvent&) override;
        bool armed = false;
    };

    using KnobLink = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ChoiceLink = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    void timerCallback() override;
    void setupKnob(juce::Slider&, const juce::String&,
                   std::unique_ptr<KnobLink>&);
    void saveWavDialog();
    void paintWave(juce::Graphics&, juce::Rectangle<float>);

    DioVerbProcessor& processor;
    DioVerbLookAndFeel look;
    juce::ComboBox mode, length, tremolo;
    std::array<std::unique_ptr<ChoiceLink>, 3> choiceLinks;
    juce::Slider size, tone, width, depth, output;
    std::array<std::unique_ptr<KnobLink>, 5> knobLinks;
    juce::TextButton capture {"CAPTURE"};
    juce::TextButton render {"RENDER"};
    juce::TextButton preview {"PREVIEW"};
    juce::TextButton save {"SAVE WAV"};
    DragPad dragPad;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::String currentStatus;
    float runningPhase = 0.0f;
    float meter = 0.0f;
    bool renderPending = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DioVerbEditor)
};
