#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>
#include <memory>

namespace {
std::unique_ptr<juce::AudioParameterFloat> floatParameter(
    const char* id, const char* label, float lo, float hi, float initial) {
    return std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{id,1}, label,
        juce::NormalisableRange<float>(lo,hi), initial);
}
std::unique_ptr<juce::AudioParameterChoice> choice(
    const char* id, const char* label, juce::StringArray options, int initial) {
    return std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{id,1}, label, std::move(options), initial);
}
}

DioVerbProcessor::DioVerbProcessor()
    : juce::AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "DIOVERB_STATE", makeParameters()) {}

juce::AudioProcessorValueTreeState::ParameterLayout DioVerbProcessor::makeParameters() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(choice("mode", "Source", {"Vocal", "Instrument"}, 0));
    layout.add(choice("length", "Reverse Length", {"1/4 bar", "1/2 bar", "1 bar"}, 1));
    layout.add(floatParameter("size", "Room Size", 0.0f, 100.0f, 58.0f));
    layout.add(floatParameter("tone", "Tone", 0.0f, 100.0f, 58.0f));
    layout.add(floatParameter("width", "Stereo Width", 0.0f, 100.0f, 78.0f));
    layout.add(choice("tremolo", "Tremolo", {"Off", "1/8", "1/16", "1/32"}, 0));
    layout.add(floatParameter("depth", "Tremolo Depth", 0.0f, 100.0f, 34.0f));
    layout.add(floatParameter("output", "Output", -18.0f, 12.0f, 0.0f));
    return layout;
}

float DioVerbProcessor::parameter(const char* id) const noexcept {
    if (auto* p = parameters.getRawParameterValue(id)) return p->load();
    return 0.0f;
}

void DioVerbProcessor::prepareToPlay(double sampleRate, int) {
    stopPreview();
    captureActive.store(false);
    captureFrames.store(0);
    capacityFrames = std::max(1, static_cast<int>(std::ceil(sampleRate * 12.0)));
    inputLeft.assign(static_cast<size_t>(capacityFrames), 0.0f);
    inputRight.assign(static_cast<size_t>(capacityFrames), 0.0f);
    inputPeak.store(0.0f);
}

void DioVerbProcessor::releaseResources() {
    stopPreview();
    captureActive.store(false);
}

bool DioVerbProcessor::isBusesLayoutSupported(const BusesLayout& layout) const {
    const auto in = layout.getMainInputChannelSet();
    return (in == juce::AudioChannelSet::mono() ||
            in == juce::AudioChannelSet::stereo())
           && in == layout.getMainOutputChannelSet();
}

void DioVerbProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const int channels = juce::jmin(buffer.getNumChannels(),
                                   getTotalNumInputChannels(), 2);
    const int n = buffer.getNumSamples();
    for (int c = channels; c < buffer.getNumChannels(); ++c) buffer.clear(c, 0, n);
    if (channels == 0 || n <= 0) return;

    if (auto* ph = getPlayHead()) {
        if (auto pos = ph->getPosition()) {
            if (auto hostBpm = pos->getBpm()) {
                if (std::isfinite(*hostBpm) && *hostBpm > 0.0)
                    bpm.store(static_cast<float>(*hostBpm), std::memory_order_relaxed);
            }
        }
    }

    const float peak = buffer.getMagnitude(0, 0, n);
    inputPeak.store(std::min(peak, 1.0f), std::memory_order_relaxed);

    // No heap work, mutexes, disk I/O or reverberation on the realtime thread.
    if (captureActive.load(std::memory_order_acquire)) {
        captureWriting.store(true, std::memory_order_release);
        if (captureActive.load(std::memory_order_acquire)) {
            const int previous = captureFrames.load(std::memory_order_relaxed);
            const int writable = std::min(n, std::max(0, capacityFrames - previous));
            if (writable > 0) {
                const float* l = buffer.getReadPointer(0);
                const float* r = buffer.getReadPointer(channels > 1 ? 1 : 0);
                juce::FloatVectorOperations::copy(inputLeft.data() + previous, l, writable);
                juce::FloatVectorOperations::copy(inputRight.data() + previous, r, writable);
                captureFrames.store(previous + writable, std::memory_order_release);
            }
            if (previous + writable >= capacityFrames)
                captureActive.store(false, std::memory_order_release);
        }
        captureWriting.store(false, std::memory_order_release);
    }

    if (!previewPlaying.load(std::memory_order_acquire)) return;
    const auto current = std::atomic_load_explicit(&previewClip, std::memory_order_acquire);
    if (!current || current->audio.getNumSamples() <= 0) {
        previewPlaying.store(false);
        return;
    }
    const int start = previewFrame.fetch_add(n, std::memory_order_acq_rel);
    const int clipLength = current->audio.getNumSamples();
    for (int c = 0; c < channels; ++c) {
        float* destination = buffer.getWritePointer(c);
        const float* source = current->audio.getReadPointer(c == 0 ? 0 : 1);
        for (int i = 0; i < n; ++i) {
            const int pos = start + i;
            destination[i] = pos < clipLength ? source[pos] : 0.0f;
        }
    }
    if (start + n >= clipLength)
        previewPlaying.store(false, std::memory_order_release);
}

bool DioVerbProcessor::startCapture() {
    if (captureWriting.load(std::memory_order_acquire) ||
        inputLeft.empty() || capacityFrames <= 0) {
        statusText = "Open the DAW audio engine before recording.";
        return false;
    }
    captureActive.store(false);
    stopPreview();
    captureFrames.store(0, std::memory_order_release);
    captureActive.store(true, std::memory_order_release);
    statusText = "CAPTURING: play your vocal or instrument. Max 12 seconds.";
    return true;
}

void DioVerbProcessor::stopCapture() {
    captureActive.store(false, std::memory_order_release);
    statusText = "Capture stopped. Click RENDER to build the reverse swell.";
}

bool DioVerbProcessor::renderCaptured() {
    captureActive.store(false, std::memory_order_release);
    if (captureWriting.load(std::memory_order_acquire)) {
        statusText = "Finishing capture... click RENDER again.";
        return false;
    }
    const int frames = std::min(capacityFrames,
                                captureFrames.load(std::memory_order_acquire));
    if (frames < 256) {
        statusText = "No audio captured. Press CAPTURE and play audio in the DAW.";
        return false;
    }
    dioverb::Settings settings;
    settings.sampleRate = getSampleRate();
    settings.bpm = getBpm();
    settings.mode = parameter("mode") < 0.5f
        ? dioverb::Mode::Vocal : dioverb::Mode::Instrument;
    settings.length = static_cast<dioverb::Length>(
        std::clamp(int(parameter("length")), 0, 2));
    settings.size = parameter("size") * 0.01f;
    settings.tone = parameter("tone") * 0.01f;
    settings.width = parameter("width") * 0.01f;
    settings.tremolo = static_cast<dioverb::Tremolo>(
        std::clamp(int(parameter("tremolo")), 0, 3));
    settings.tremoloDepth = parameter("depth") * 0.01f;
    settings.outputDb = parameter("output");
    auto rendered = dioverb::render(inputLeft.data(), inputRight.data(),
                                    static_cast<size_t>(frames), settings);
    if (!rendered.valid()) {
        statusText = "No usable signal found. Capture a louder phrase.";
        return false;
    }

    auto fresh = std::make_shared<PreviewClip>();
    fresh->audio.setSize(2, static_cast<int>(rendered.size()));
    juce::FloatVectorOperations::copy(fresh->audio.getWritePointer(0),
                                     rendered.left.data(), fresh->audio.getNumSamples());
    juce::FloatVectorOperations::copy(fresh->audio.getWritePointer(1),
                                     rendered.right.data(), fresh->audio.getNumSamples());

    const auto outputDir = juce::File::getSpecialLocation(
        juce::File::userApplicationDataDirectory)
        .getChildFile("DioVERB").getChildFile("Exports");
    if (outputDir.createDirectory().failed()) {
        statusText = "Cannot create DioVERB export directory.";
        return false;
    }
    fresh->wav = outputDir.getNonexistentChildFile(
        "DioVERB_" + juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S")
        + "_" + juce::String(++exportSequence), ".wav", false);

    auto fileStream = std::unique_ptr<juce::FileOutputStream>(
        fresh->wav.createOutputStream());
    if (!fileStream || !fileStream->openedOk()) {
        statusText = "Cannot write WAV file to Application Support.";
        return false;
    }
    juce::WavAudioFormat wav;
    auto writer = std::unique_ptr<juce::AudioFormatWriter>(
        wav.createWriterFor(fileStream.get(), settings.sampleRate, 2, 24, {}, 0));
    if (!writer) {
        statusText = "Failed to initialize 24-bit WAV writer.";
        return false;
    }
    fileStream.release(); // JUCE AudioFormatWriter now owns and closes the stream.
    const bool ok = writer->writeFromAudioSampleBuffer(
        fresh->audio, 0, fresh->audio.getNumSamples());
    writer.reset();
    if (!ok || !fresh->wav.existsAsFile()) {
        statusText = "WAV rendering failed.";
        return false;
    }

    stopPreview();
    std::atomic_store_explicit(
        &previewClip, std::shared_ptr<const PreviewClip>(std::move(fresh)),
        std::memory_order_release);
    statusText = "READY: preview the swell or drag its WAV to FL Studio.";
    return true;
}

void DioVerbProcessor::startPreview() {
    if (!hasRenderedAudio()) return;
    previewFrame.store(0, std::memory_order_release);
    previewPlaying.store(true, std::memory_order_release);
}
void DioVerbProcessor::stopPreview() {
    previewPlaying.store(false, std::memory_order_release);
    previewFrame.store(0, std::memory_order_release);
}
bool DioVerbProcessor::hasRenderedAudio() const noexcept {
    return static_cast<bool>(std::atomic_load_explicit(
        &previewClip, std::memory_order_acquire));
}
float DioVerbProcessor::previewProgress() const noexcept {
    const auto current = std::atomic_load_explicit(&previewClip, std::memory_order_acquire);
    if (!current || current->audio.getNumSamples() < 1) return 0.0f;
    return std::clamp(float(previewFrame.load()) / float(current->audio.getNumSamples()),
                      0.0f, 1.0f);
}
juce::File DioVerbProcessor::getLatestWav() const {
    const auto current = std::atomic_load_explicit(&previewClip, std::memory_order_acquire);
    return current ? current->wav : juce::File();
}

void DioVerbProcessor::getStateInformation(juce::MemoryBlock& dest) {
    if (auto xml = parameters.copyState().createXml()) copyXmlToBinary(*xml, dest);
}
void DioVerbProcessor::setStateInformation(const void* source, int size) {
    if (auto xml = getXmlFromBinary(source, size)) {
        auto state = juce::ValueTree::fromXml(*xml);
        if (state.isValid() && state.hasType(parameters.state.getType()))
            parameters.replaceState(state);
    }
}
juce::AudioProcessorEditor* DioVerbProcessor::createEditor() {
    return new DioVerbEditor(*this);
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new DioVerbProcessor();
}
