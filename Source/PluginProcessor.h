#pragma once
#include <JuceHeader.h>
#include "DSP.h"

class DC23AudioProcessor final : public juce::AudioProcessor {
public:
    DC23AudioProcessor();
    ~DC23AudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "DC-23"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "DC-23"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // Called from the message thread; also supports host automation.
    void applyFactoryPreset(int index);
    juce::AudioProcessorValueTreeState parameters;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameterLayout();
    float read(const char* id) const noexcept;
    dc23::Engine engine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DC23AudioProcessor)
};
