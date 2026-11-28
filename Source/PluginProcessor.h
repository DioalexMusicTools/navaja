#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <memory>
#include "Harmony.h"

class DioChordProcessor final : public juce::AudioProcessor {
public:
    DioChordProcessor();
    ~DioChordProcessor() override = default;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "DioCHORD X"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.5; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;

    float getHostBpm() const noexcept {return currentBpm.load(std::memory_order_relaxed);}
    float getPlayPosition() const noexcept {return currentBeat.load(std::memory_order_relaxed);}
    bool isPreviewing() const noexcept {return previewActive.load(std::memory_order_relaxed);}
    void togglePreview();
    void stopPreview();
    bool exportMidiFile(const juce::File&);
    juce::File exportMidiToTemporaryFile();
    diochord::Settings currentSettings(float bpmOverride=0.0f) const noexcept;
    juce::AudioProcessorValueTreeState parameters;

private:
    struct Voice {
        double phase=0.0, frequency=440.0;
        float envelope=0.0f, velocity=0.0f;
        int pitch=-1;
        bool releasing=true;
        uint32_t serial=0;
    };
    struct Trigger {
        int sample=0,pitch=60,velocity=90;
        bool on=true;
    };
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();
    float raw(const char* id) const noexcept;
    void noteOn(int pitch,int velocity) noexcept;
    void noteOff(int pitch) noexcept;
    void stopAllNotes() noexcept;
    float synthSample(int instrument) noexcept;
    void queueTriggers(const diochord::Sequence&, double startBeat,
                       double endBeat, double samplesPerBeat, int blockSize,
                       std::array<Trigger,512>& triggers,int& count) const noexcept;

    std::array<Voice,48> voices {};
    uint32_t serial=1;
    std::atomic<float> currentBpm {120.0f}, currentBeat{0.0f};
    std::atomic<bool> previewActive{false}, restartPreview{false};
    float freeBeat=0.0f;
    bool previousPlaying=false;
    int lastPreset=-1,lastKey=-1,lastMode=-1,lastFeel=-1;
    double clockRate=44100.0;
    std::atomic<int> exportIndex{0};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DioChordProcessor)
};
