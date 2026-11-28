#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FactoryPresets.h"
#include "Timing.h"
#include <array>
#include <memory>

namespace {
using APF = juce::AudioParameterFloat;
using APB = juce::AudioParameterBool;

// Make units and <= 2 decimal precision available to VST3 hosts as well as the GUI.
juce::AudioParameterFloatAttributes valueAttributes(const char* id) {
    const juce::String key(id);
    juce::String suffix = "%";
    if (key == "wobbleRate" || key == "lowCut" || key == "highCut")
        suffix = "Hz";
    else if (key == "magneticSpeed" || key == "spaceSize")
        suffix = "ms";
    else if (key == "outputDb" || key == "distortionTone")
        suffix = "dB";

    return juce::AudioParameterFloatAttributes()
        .withLabel(suffix)
        .withStringFromValueFunction([](float value, int) {
            auto result = juce::String(double(value), 2);
            while (result.containsChar('.') && result.endsWithChar('0'))
                result = result.dropLastCharacters(1);
            if (result.endsWithChar('.'))
                result = result.dropLastCharacters(1);
            return result;
        })
        .withValueFromStringFunction([](const juce::String& value) {
            return value.retainCharacters("0123456789.-").getFloatValue();
        });
}
std::unique_ptr<APF> param(const char* id, const char* name,
                           float low, float high, float initial) {
    return std::make_unique<APF>(juce::ParameterID(id, 1), name,
                                 juce::NormalisableRange<float>(low, high),
                                 initial, valueAttributes(id));
}
// Log-like taper makes 20–20000Hz controls usable at low frequencies.
std::unique_ptr<APF> cutoffParam(const char* id, const char* name, float initial) {
    return std::make_unique<APF>(juce::ParameterID(id, 1), name,
                                 juce::NormalisableRange<float>(20.0f, 20000.0f,
                                                               0.0f, 0.28f),
                                 initial, valueAttributes(id));
}
std::unique_ptr<APB> toggle(const char* id, const char* name, bool defaultState = true) {
    return std::make_unique<APB>(juce::ParameterID(id, 1), name, defaultState);
}
}

DC23AudioProcessor::DC23AudioProcessor()
    : juce::AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "DC23STATE", makeParameterLayout()) {}

juce::AudioProcessorValueTreeState::ParameterLayout DC23AudioProcessor::makeParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    // Explicit engineering units; the control values in FL Studio are meaningful.
    layout.add(param("noise", "Noise Amount", 0, 100, 0));
    layout.add(param("wobble", "Wobble Amount", 0, 100, 0));
    layout.add(param("distort", "Distort Amount", 0, 100, 0));
    layout.add(param("digital", "Digital Amount", 0, 100, 0));
    layout.add(param("space", "Space Amount", 0, 100, 0));
    layout.add(param("magnetic", "Magnetic Amount", 0, 100, 0));

    layout.add(toggle("noiseOn", "Noise Enabled"));
    layout.add(toggle("wobbleOn", "Wobble Enabled"));
    layout.add(toggle("distortOn", "Distort Enabled"));
    layout.add(toggle("digitalOn", "Digital Enabled"));
    layout.add(toggle("spaceOn", "Space Enabled"));
    layout.add(toggle("magneticOn", "Magnetic Enabled"));

    layout.add(param("noiseFlux", "Noise Flux", 0, 100, 0));
    layout.add(param("wobbleFlux", "Wobble Flux", 0, 100, 0));
    layout.add(param("distortFlux", "Distort Flux", 0, 100, 0));
    layout.add(param("digitalFlux", "Digital Flux", 0, 100, 0));
    layout.add(param("spaceFlux", "Space Flux", 0, 100, 0));
    layout.add(param("magneticFlux", "Magnetic Flux", 0, 100, 0));

    layout.add(param("noiseColor", "Noise Color (%)", 0, 45, 35));
    layout.add(param("wobbleRate", "Wobble Rate (Hz)", 6.0f, 30.0f, 12.0f));
    layout.add(param("distortionTone", "Distortion Tone (dB)", -12.0f, 12.0f, 4.2f));
    layout.add(param("digitalRate", "Digital Crush (%)", 0, 50, 25));
    layout.add(param("digitalSmooth", "Digital Smooth", 0, 100, 55));
    layout.add(param("spaceSize", "Reverb Decay (ms)", 0, 100, 50));
    layout.add(param("magneticSpeed", "Magnetic Speed (ms)", 0, 100, 50));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("noiseStyle", 1), "Noise Texture",
        juce::StringArray{"Tape", "Vinyl", "Hum"}, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("distortionStyle", 1), "Distortion Character",
        juce::StringArray{"Tape", "Valve", "Fuzz"}, 0));

    layout.add(param("magnitude", "Master Amount", 0, 100, 100));
    // Unipolar, zero = entirely dry, 100 = entirely wet.
    layout.add(param("mix", "Master Dry Wet (%)", 0, 100, 50));
    layout.add(param("outputDb", "Output (dB)", -18, 18, 5));
    layout.add(param("width", "Stereo Width (%)", 0, 200, 100));
    layout.add(cutoffParam("lowCut", "Low Cut (Hz)", 62.0f));
    layout.add(cutoffParam("highCut", "High Cut (Hz)", 8778.0f));
    layout.add(toggle("lowCutOn", "Low Cut Enabled", true));
    layout.add(toggle("highCutOn", "High Cut Enabled", true));

    layout.add(toggle("echoOn", "Echo Enabled", false));
    layout.add(param("echoMix", "Echo Dry Wet (%)", 0, 100, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("echoDivision", 1), "Echo Note Division",
        juce::StringArray{"1/2", "1/4", "1/8 D", "1/8", "1/8 T", "1/16", "1/32"}, 3));
    layout.add(param("echoFeedback", "Echo Feedback", 0, 100, 35));
    return layout;
}

bool DC23AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto in = layouts.getMainInputChannelSet();
    return (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo())
        && layouts.getMainOutputChannelSet() == in;
}

void DC23AudioProcessor::prepareToPlay(double sampleRate, int) {
    engine.prepare(sampleRate);
}
void DC23AudioProcessor::releaseResources() { engine.reset(); }

float DC23AudioProcessor::read(const char* id) const noexcept {
    if (auto* atomic = parameters.getRawParameterValue(id)) return atomic->load();
    return 0.0f;
}

void DC23AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const int used = juce::jmin(buffer.getNumChannels(), getTotalNumInputChannels(), 2);
    for (int c = used; c < buffer.getNumChannels(); ++c)
        buffer.clear(c, 0, buffer.getNumSamples());
    if (used == 0 || buffer.getNumSamples() == 0) return;

    if (auto* playHead = getPlayHead()) {
        if (auto position = playHead->getPosition()) {
            if (auto bpm = position->getBpm()) {
                if (std::isfinite(*bpm) && *bpm > 0.0)
                    hostBpm.store(static_cast<float>(*bpm), std::memory_order_relaxed);
            }
        }
    }

    dc23::Parameters p;
    constexpr std::array<const char*, 6> amounts {
        "noise", "wobble", "distort", "digital", "space", "magnetic"
    };
    constexpr std::array<const char*, 6> toggles {
        "noiseOn", "wobbleOn", "distortOn", "digitalOn", "spaceOn", "magneticOn"
    };
    constexpr std::array<const char*, 6> fluxes {
        "noiseFlux", "wobbleFlux", "distortFlux", "digitalFlux", "spaceFlux", "magneticFlux"
    };
    for (size_t i = 0; i < amounts.size(); ++i) {
        p.amount[i] = read(amounts[i]) * 0.01f;
        p.enabled[i] = read(toggles[i]) > 0.5f;
        p.flux[i] = read(fluxes[i]) * 0.01f;
    }
    p.noiseStyle = static_cast<int>(read("noiseStyle"));
    p.distortionStyle = static_cast<int>(read("distortionStyle"));
    p.noiseColor = read("noiseColor") * 0.01f;
    p.wobbleRate = read("wobbleRate");
    p.distortionTone = read("distortionTone");
    p.digitalRate = read("digitalRate") * 0.02f;
    p.digitalSmooth = read("digitalSmooth") * 0.01f;
    p.spaceDecayMs = read("spaceSize");
    p.magneticIntervalMs = read("magneticSpeed");
    p.magnitude = read("magnitude") * 0.01f;
    p.mix = read("mix") * 0.01f;
    p.outputDb = read("outputDb");
    p.width = read("width") * 0.01f;
    p.lowCutHz = read("lowCutOn") > 0.5f ? read("lowCut") : 0.0f;
    p.highCutHz = read("highCutOn") > 0.5f ? read("highCut") : 0.0f;

    p.echoMix = read("echoOn") > 0.5f
        ? read("echoMix") * 0.01f : 0.0f;
    p.echoFeedback = read("echoFeedback") * 0.008f;
    p.echoTimeMs = dc23::syncedEchoMilliseconds(
        static_cast<int>(read("echoDivision")), getHostBpm());

    float* ptr[2] { buffer.getWritePointer(0),
                    used > 1 ? buffer.getWritePointer(1) : nullptr };
    engine.process(ptr, used, buffer.getNumSamples(), p);

    float peak = 0.0f;
    for (int c = 0; c < used; ++c)
        peak = std::max(peak, buffer.getMagnitude(c, 0, buffer.getNumSamples()));
    outputPeak.store(std::min(peak, 2.0f), std::memory_order_relaxed);
}

void DC23AudioProcessor::getStateInformation(juce::MemoryBlock& dest) {
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary(*xml, dest);
}

void DC23AudioProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size)) {
        auto tree = juce::ValueTree::fromXml(*xml);
        if (tree.isValid() && tree.hasType(parameters.state.getType()))
            parameters.replaceState(tree);
    }
}

void DC23AudioProcessor::applyFactoryPreset(int index) {
    const auto& list = dc23::factoryPresets();
    if (index < 0 || index >= static_cast<int>(list.size())) return;
    const auto& p = list[static_cast<size_t>(index)];
    auto set = [this](const char* id, float value) {
        if (auto* par = parameters.getParameter(id)) {
            par->beginChangeGesture();
            par->setValueNotifyingHost(par->convertTo0to1(value));
            par->endChangeGesture();
        }
    };
    constexpr std::array<const char*, 6> amountIds {
        "noise", "wobble", "distort", "digital", "space", "magnetic"
    };
    constexpr std::array<const char*, 6> fluxIds {
        "noiseFlux", "wobbleFlux", "distortFlux", "digitalFlux", "spaceFlux", "magneticFlux"
    };
    constexpr std::array<const char*, 6> enableIds {
        "noiseOn", "wobbleOn", "distortOn", "digitalOn", "spaceOn", "magneticOn"
    };
    for (size_t i = 0; i < 6; ++i) {
        set(amountIds[i], p.amount[i]);
        set(fluxIds[i], p.flux[i]);
        set(enableIds[i], 1.0f);
    }
    set("noiseStyle", float(p.noiseStyle));
    set("distortionStyle", float(p.distortionStyle));
    set("noiseColor", p.noiseColor);
    set("wobbleRate", p.wobbleHz);
    set("distortionTone", p.driveTone);
    set("digitalRate", p.digitalRate);
    set("digitalSmooth", p.digitalSmooth);
    set("spaceSize", p.decayMs);
    set("magneticSpeed", p.wearMs);
    set("magnitude", p.magnitude);
    set("mix", p.mix);
    set("outputDb", 5.0f);
    set("width", p.width);
    set("lowCutOn", p.lowCutOn ? 1.0f : 0.0f);
    set("highCutOn", p.highCutOn ? 1.0f : 0.0f);
    set("lowCut", p.lowCutHz);
    set("highCut", p.highCutHz);
    set("echoOn", p.echoOn ? 1.0f : 0.0f);
    set("echoMix", p.echoMix);
    set("echoDivision", float(p.division));
    set("echoFeedback", p.feedback);
}

juce::AudioProcessorEditor* DC23AudioProcessor::createEditor() {
    return new DC23AudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new DC23AudioProcessor();
}
