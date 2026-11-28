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

// Draw the gear without depending on an OS font having a gear glyph.
class DC23SettingsGear final : public juce::Button {
public:
    DC23SettingsGear() : juce::Button("DC-23 Settings") {}
    void paintButton(juce::Graphics&, bool highlighted, bool down) override;
};

class DC23SettingsPanel final : public juce::Component {
public:
    DC23SettingsPanel();
    void paint(juce::Graphics&) override;
    void resized() override;
    juce::Slider scaleControl;
    juce::TextButton aboutButton {"ABOUT"};
};

class DC23AudioProcessorEditor final : public juce::AudioProcessorEditor,
                                        private juce::Timer {
public:
    explicit DC23AudioProcessorEditor(DC23AudioProcessor&);
    ~DC23AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderLink = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonLink = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboLink = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    struct Module {
        juce::Slider amount, character, flux;
        juce::ToggleButton enabled;
        std::unique_ptr<SliderLink> amountLink, characterLink, fluxLink;
        std::unique_ptr<ButtonLink> enabledLink;
    };

    void setupKnob(juce::Slider&, const juce::String& id,
                   std::unique_ptr<SliderLink>& attachment,
                   bool bipolar = false);
    void openPreset(bool save);
    void setInterfaceScale(int percent, bool saveSetting);
    void showAbout();
    juce::Rectangle<int> scaledBounds(int x, int y, int w, int h) const;
    void timerCallback() override;
    void drawModulePlot(juce::Graphics&, int index, juce::Rectangle<float> bounds);
    static juce::Rectangle<int> moduleBounds(int index);

    DC23AudioProcessor& processor;
    DC23LookAndFeel look;
    std::array<Module, 6> modules;
    juce::Slider magnitude, mix, output, width, lowCut, highCut;
    juce::Slider echoMix, echoFeedback, digitalSmooth;
    std::array<std::unique_ptr<SliderLink>, 9> masterLinks;

    juce::ToggleButton echoEnabled, lowEnabled, highEnabled;
    std::unique_ptr<ButtonLink> echoEnabledLink, lowEnabledLink, highEnabledLink;

    juce::ComboBox factoryPresets, noiseStyle, distortionStyle, echoDivision;
    std::unique_ptr<ComboLink> noiseStyleLink, distortionStyleLink, echoDivisionLink;
    juce::TextButton saveButton {"SAVE"}, loadButton {"LOAD"};
    DC23SettingsGear settingsGear;
    DC23SettingsPanel settingsPanel;
    int scalePercent = 100;
    bool settingsOpen = false;
    std::unique_ptr<juce::FileChooser> chooser;
    float animation = 0.0f, meterDisplay = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DC23AudioProcessorEditor)
};
