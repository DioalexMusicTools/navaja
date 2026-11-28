#pragma once
#include <JuceHeader.h>
#include <array>
#include <memory>
#include "PluginProcessor.h"

class DioChordLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    DioChordLookAndFeel();
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
};

class DioChordEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit DioChordEditor(DioChordProcessor&);
    ~DioChordEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    struct DragPad final : juce::Component {
        DioChordProcessor* processor=nullptr;
        bool armed=false;
        void paint(juce::Graphics&) override;
        void mouseDown(const juce::MouseEvent&) override {armed=true;}
        void mouseDrag(const juce::MouseEvent&) override;
    };
    using KnobLink=juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboLink=juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    void timerCallback() override;
    void setParameter(const char* id,float actual);
    void updateCards();
    void setupKnob(juce::Slider&,const char*,std::unique_ptr<KnobLink>&);
    void saveMidiDialog();

    DioChordProcessor& processor;
    DioChordLookAndFeel look;
    std::array<juce::TextButton,4> moodButtons;
    std::array<juce::TextButton,4> progressionButtons;
    juce::ComboBox key,scale,octave,inversion,speed,feel,sound;
    std::array<std::unique_ptr<ComboLink>,7> comboLinks;
    juce::Slider humanize,strum,velocity,level;
    std::array<std::unique_ptr<KnobLink>,4> knobLinks;
    juce::TextButton play {"PLAY"}, newTake {"NEW TAKE"}, save {"SAVE MIDI"};
    DragPad dragPad;
    std::unique_ptr<juce::FileChooser> chooser;
    int visibleMood=2, lastPreset=-1;
    float cursor=0.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DioChordEditor)
};
