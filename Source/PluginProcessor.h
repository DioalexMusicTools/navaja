#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>
#include "ReverseEngine.h"

class DioVerbProcessor final : public juce::AudioProcessor {
public:
    DioVerbProcessor();
    ~DioVerbProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "DioVERB"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& data) override;
    void setStateInformation(const void* data, int size) override;

    // Must only be invoked from the message thread.
    bool startCapture();
    void stopCapture();
    bool renderCaptured();
    void startPreview();
    void stopPreview();

    bool isCapturing() const noexcept { return captureActive.load(); }
    bool isCaptureWriterBusy() const noexcept { return captureWriting.load(); }
    int getCapturedFrames() const noexcept { return captureFrames.load(); }
    int getCaptureCapacity() const noexcept { return capacityFrames; }
    float getBpm() const noexcept { return bpm.load(); }
    float getInputPeak() const noexcept { return inputPeak.load(); }
    bool isPreviewPlaying() const noexcept { return previewPlaying.load(); }
    float previewProgress() const noexcept;
    bool hasRenderedAudio() const noexcept;
    juce::File getLatestWav() const;
    juce::String getStatus() const { return statusText; }

    juce::AudioProcessorValueTreeState parameters;

private:
    struct PreviewClip {
        juce::AudioBuffer<float> audio;
        juce::File wav;
    };
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
    float parameter(const char* id) const noexcept;

    std::vector<float> inputLeft, inputRight;
    int capacityFrames = 0;
    std::atomic<int> captureFrames {0};
    std::atomic<bool> captureActive {false}, captureWriting {false};
    std::atomic<float> bpm {120.0f}, inputPeak {0.0f};
    std::atomic<bool> previewPlaying {false};
    std::atomic<int> previewFrame {0};
    std::shared_ptr<const PreviewClip> previewClip;
    juce::String statusText = "Press CAPTURE, then play a phrase in your DAW.";
    int exportSequence = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DioVerbProcessor)
};
