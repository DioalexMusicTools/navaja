#pragma once
#include <JuceHeader.h>
#include <atomic>
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
    float getHostBpm() const noexcept { return hostBpm.load(std::memory_order_relaxed); }
    float getOutputPeak() const noexcept { return outputPeak.load(std::memory_order_relaxed); }
    juce::AudioProcessorValueTreeState parameters;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameterLayout();
    float read(const char* id) const noexcept;
    dc23::Engine engine;
    std::atomic<float> hostBpm {120.0f};
    std::atomic<float> outputPeak {0.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DC23AudioProcessor)
};
