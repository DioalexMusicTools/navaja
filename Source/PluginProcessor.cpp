#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <array>
#include <memory>

namespace {
using APF = juce::AudioParameterFloat;
using APB = juce::AudioParameterBool;

std::unique_ptr<APF> param(const char* id, const char* name,
                           float low, float high, float initial) {
    return std::make_unique<APF>(juce::ParameterID(id, 1), name,
                                 juce::NormalisableRange<float>(low, high), initial);
}
std::unique_ptr<APB> toggle(const char* id, const char* name) {
    return std::make_unique<APB>(juce::ParameterID(id, 1), name, true);
}
}

DC23AudioProcessor::DC23AudioProcessor()
    : juce::AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "DC23STATE", makeParameterLayout()) {}

juce::AudioProcessorValueTreeState::ParameterLayout DC23AudioProcessor::makeParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(param("noise", "Noise", 0, 1, 0.15f));
    layout.add(param("wobble", "Wobble", 0, 1, 0.20f));
    layout.add(param("distort", "Distort", 0, 1, 0.15f));
    layout.add(param("digital", "Digital", 0, 1, 0.0f));
    layout.add(param("space", "Space", 0, 1, 0.10f));
    layout.add(param("magnetic", "Magnetic", 0, 1, 0.10f));

    layout.add(toggle("noiseOn", "Noise Enable"));
    layout.add(toggle("wobbleOn", "Wobble Enable"));
    layout.add(toggle("distortOn", "Distort Enable"));
    layout.add(toggle("digitalOn", "Digital Enable"));
    layout.add(toggle("spaceOn", "Space Enable"));
    layout.add(toggle("magneticOn", "Magnetic Enable"));

    layout.add(param("noiseColor", "Noise Color", 0, 1, 0.35f));
    layout.add(param("wobbleRate", "Wobble Rate", 0.1f, 8.0f, 0.9f));
    layout.add(param("distortionTone", "Drive Color", 0, 1, 0.5f));
    layout.add(param("digitalRate", "Rate Crush", 0, 1, 0.5f));
    layout.add(param("spaceSize", "Room Size", 0, 1, 0.5f));
    layout.add(param("magneticSpeed", "Wear Speed", 0, 1, 0.5f));

    // Flux changes the subtle time-variation independently for each effect.
    layout.add(param("noiseFlux", "Noise Flux", 0, 1, 0.12f));
    layout.add(param("wobbleFlux", "Wobble Flux", 0, 1, 0.10f));
    layout.add(param("distortFlux", "Distort Flux", 0, 1, 0.07f));
    layout.add(param("digitalFlux", "Digital Flux", 0, 1, 0.08f));
    layout.add(param("spaceFlux", "Space Flux", 0, 1, 0.05f));
    layout.add(param("magneticFlux", "Magnetic Flux", 0, 1, 0.30f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("noiseStyle", 1), "Noise Texture",
        juce::StringArray {"Tape", "Vinyl", "Hum"}, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("distortionStyle", 1), "Distortion Character",
        juce::StringArray {"Tape", "Valve", "Fuzz"}, 0));
    layout.add(param("digitalSmooth", "Digital Smooth", 0, 1, 0.55f));
    layout.add(param("width", "Stereo Width", 0, 2, 1.0f));
    layout.add(param("echoMix", "Echo Mix", 0, 1, 0.0f));
    layout.add(param("echoTimeMs", "Echo Time", 90, 900, 290.0f));
    layout.add(param("echoFeedback", "Echo Feedback", 0, 0.8f, 0.35f));

    layout.add(param("magnitude", "Magnitude", 0, 1, 1.0f));
    layout.add(param("mix", "Dry Wet", 0, 1, 1.0f));
    layout.add(param("outputDb", "Output dB", -18.0f, 12.0f, 0.0f));
    layout.add(param("lowCut", "Low Cut", 0, 1, 0.0f));
    layout.add(param("highCut", "High Cut", 0, 1, 0.0f));
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
    for (int c = used; c < buffer.getNumChannels(); ++c) buffer.clear(c, 0, buffer.getNumSamples());
    if (used == 0) return;

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
        p.amount[i] = read(amounts[i]);
        p.enabled[i] = read(toggles[i]) > 0.5f;
        p.flux[i] = read(fluxes[i]);
    }
    p.noiseColor = read("noiseColor");
    p.wobbleRate = read("wobbleRate");
    p.distortionTone = read("distortionTone");
    p.digitalRate = read("digitalRate");
    p.spaceSize = read("spaceSize");
    p.magneticSpeed = read("magneticSpeed");
    p.magnitude = read("magnitude");
    p.mix = read("mix");
    p.outputDb = read("outputDb");
    p.lowCut = read("lowCut");
    p.highCut = read("highCut");
    p.noiseStyle = static_cast<int>(read("noiseStyle"));
    p.distortionStyle = static_cast<int>(read("distortionStyle"));
    p.digitalSmooth = read("digitalSmooth");
    p.width = read("width");
    p.echoMix = read("echoMix");
    p.echoTimeMs = read("echoTimeMs");
    p.echoFeedback = read("echoFeedback");

    float* ptr[2] { buffer.getWritePointer(0), used > 1 ? buffer.getWritePointer(1) : nullptr };
    engine.process(ptr, used, buffer.getNumSamples(), p);
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
    static constexpr std::array<std::array<float, 6>, 6> presets {{
        {{ 0.15f, 0.20f, 0.15f, 0.0f, 0.10f, 0.10f }}, // Init
        {{ 0.40f, 0.43f, 0.35f, 0.22f, 0.15f, 0.41f }}, // VHS Glow
        {{ 0.45f, 0.12f, 0.48f, 0.27f, 0.33f, 0.23f }}, // Dusty Keys
        {{ 0.08f, 0.16f, 0.64f, 0.50f, 0.12f, 0.35f }}, // Drum Wear
        {{ 0.24f, 0.70f, 0.17f, 0.10f, 0.50f, 0.55f }}, // Tape Drift
        {{ 0.54f, 0.21f, 0.83f, 0.86f, 0.06f, 0.68f }}  // Broken Radio
    }};
    if (index < 0 || index >= static_cast<int>(presets.size())) return;
    constexpr std::array<const char*, 6> ids {
        "noise", "wobble", "distort", "digital", "space", "magnetic"
    };
    auto set = [this](const char* id, float value) {
        if (auto* param = parameters.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(value));
    };
    for (size_t i = 0; i < ids.size(); ++i) set(ids[i], presets[static_cast<size_t>(index)][i]);
    set("magnitude", 1.0f);
    set("mix", 1.0f);
    set("outputDb", 0.0f);
    set("lowCut", index == 5 ? 0.28f : 0.0f);
    set("highCut", index == 5 ? 0.58f : (index == 1 ? 0.30f : 0.0f));
    constexpr std::array<const char*, 6> fluxIds {
        "noiseFlux", "wobbleFlux", "distortFlux", "digitalFlux", "spaceFlux", "magneticFlux"
    };
    constexpr std::array<float, 6> baseFlux { 0.12f, 0.10f, 0.07f, 0.08f, 0.05f, 0.30f };
    for (size_t i = 0; i < fluxIds.size(); ++i) {
        const float dramatic = (index == 4 || index == 5) ? 0.34f : 0.10f;
        set(fluxIds[i], std::clamp(baseFlux[i] + dramatic, 0.0f, 1.0f));
    }
    set("noiseStyle", index == 2 ? 1.0f : (index == 5 ? 2.0f : 0.0f));
    set("distortionStyle", index == 3 ? 1.0f : (index == 5 ? 2.0f : 0.0f));
    set("digitalSmooth", index == 5 ? 0.1f : 0.60f);
    set("width", index == 4 ? 1.40f : 1.0f);
    set("echoMix", index == 4 ? 0.26f : (index == 1 ? 0.13f : 0.0f));
    set("echoFeedback", index == 4 ? 0.45f : 0.35f);
    set("echoTimeMs", 290.0f);

}

juce::AudioProcessorEditor* DC23AudioProcessor::createEditor() {
    return new DC23AudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new DC23AudioProcessor();
}
