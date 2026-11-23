#pragma once

// Original JUCE-independent DC-23 DSP. All allocations happen in prepare().
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace dc23 {
constexpr float pi = 3.14159265358979323846f;
inline float clamp01(float v) noexcept { return std::clamp(v, 0.0f, 1.0f); }

struct Parameters {
    std::array<float, 6> amount {{ 0.15f, 0.2f, 0.15f, 0.0f, 0.10f, 0.10f }};
    std::array<bool, 6> enabled {{ true, true, true, true, true, true }};
    float magnitude = 1.0f;
    float mix = 1.0f;
    float outputDb = 0.0f;
    float noiseColor = 0.35f;
    float wobbleRate = 0.9f;
    float distortionTone = 0.5f;
    float digitalRate = 0.5f;
    float spaceSize = 0.5f;
    float magneticSpeed = 0.5f;
    float lowCut = 0.0f;
    float highCut = 0.0f;
};

class Engine {
public:
    void prepare(double newSampleRate) {
        rate = std::max(8000.0f, static_cast<float>(newSampleRate));
        for (int c = 0; c < 2; ++c) {
            auto& ch = channels[static_cast<size_t>(c)];
            ch.wowDelay.assign(static_cast<size_t>(rate * 0.065f) + 8u, 0.0f);
            const float lengths[3] = { 0.037f, 0.053f, 0.071f };
            for (int k = 0; k < 3; ++k)
                ch.reverb[static_cast<size_t>(k)].assign(
                    static_cast<size_t>(rate * (lengths[k] + c * 0.0017f)) + 1u, 0.0f);
        }
        reset();
    }

    void reset() noexcept {
        for (auto& ch : channels) {
            std::fill(ch.wowDelay.begin(), ch.wowDelay.end(), 0.0f);
            for (auto& comb : ch.reverb) std::fill(comb.begin(), comb.end(), 0.0f);
            ch.wowWrite = 0;
            ch.revWrite = {{0, 0, 0}};
            ch.revDamped = {{0.0f, 0.0f, 0.0f}};
            ch.noiseLP = ch.hpMemory = ch.lpMemory = 0.0f;
            ch.held = 0.0f;
            ch.holdCounter = 0;
            ch.dropCounter = 0;
            ch.dropTarget = ch.dropLevel = 1.0f;
        }
        smoothed.fill(0.0f);
        wowPhase = flutterPhase = 0.0f;
        rng = 0x6923a91bu;
    }

    void process(float* const* data, int channelCount, int sampleCount, const Parameters& p) noexcept {
        if (sampleCount <= 0 || data == nullptr) return;
        channelCount = std::clamp(channelCount, 0, 2);
        if (channelCount == 0) return;

        std::array<float, 6> targets {};
        for (size_t n = 0; n < targets.size(); ++n)
            targets[n] = clamp01(p.amount[n]) * (p.enabled[n] ? 1.0f : 0.0f) * clamp01(p.magnitude);

        const float slew = std::min(1.0f, 1.0f / (0.008f * rate));
        const float mix = clamp01(p.mix);
        const float outputGain = std::pow(10.0f, std::clamp(p.outputDb, -30.0f, 18.0f) / 20.0f);
        const float hpFreq = 25.0f + 1700.0f * p.lowCut * p.lowCut;
        const float lpFreq = std::clamp(19000.0f - 17500.0f * p.highCut * p.highCut, 200.0f, 0.45f * rate);
        const float hpA = std::exp(-2.0f * pi * std::min(hpFreq, rate * 0.45f) / rate);
        const float lpA = std::exp(-2.0f * pi * lpFreq / rate);
        const float wowAdvance = 2.0f * pi * std::clamp(p.wobbleRate, 0.1f, 8.0f) / rate;
        const float flutterAdvance = 2.0f * pi * (9.0f + 2.0f * clamp01(p.magneticSpeed)) / rate;

        for (int i = 0; i < sampleCount; ++i) {
            for (size_t n = 0; n < targets.size(); ++n)
                smoothed[n] += (targets[n] - smoothed[n]) * slew;

            const float slowLfo = std::sin(wowPhase);
            const float fastLfo = std::sin(flutterPhase);
            wowPhase += wowAdvance;
            flutterPhase += flutterAdvance;
            if (wowPhase >= 2.0f * pi) wowPhase -= 2.0f * pi;
            if (flutterPhase >= 2.0f * pi) flutterPhase -= 2.0f * pi;

            for (int c = 0; c < channelCount; ++c) {
                auto& ch = channels[static_cast<size_t>(c)];
                const float dry = data[c][i];
                float x = dry;

                // Tape hiss / filtered dust.
                const float white = randomSigned();
                ch.noiseLP = 0.975f * ch.noiseLP + 0.025f * white;
                const float n = (1.0f - clamp01(p.noiseColor)) * white
                              + (4.0f * clamp01(p.noiseColor)) * ch.noiseLP;
                x += n * (0.085f * smoothed[0] * smoothed[0]);

                // Wow and flutter: continuously updated fractional-delay line.
                ch.wowDelay[ch.wowWrite] = x;
                if (smoothed[1] > 0.00001f) {
                    const float modulation = 0.0015f * slowLfo + 0.00018f * fastLfo;
                    const float seconds = 0.008f + smoothed[1] * modulation;
                    const float delaySamples = seconds * rate;
                    float read = static_cast<float>(ch.wowWrite) - delaySamples;
                    const auto size = static_cast<float>(ch.wowDelay.size());
                    while (read < 0.0f) read += size;
                    const size_t a = static_cast<size_t>(read);
                    const size_t b = (a + 1u) % ch.wowDelay.size();
                    const float frac = read - static_cast<float>(a);
                    const float warped = ch.wowDelay[a] * (1.0f - frac) + ch.wowDelay[b] * frac;
                    x = x + (warped - x) * smoothed[1];
                }
                ch.wowWrite = (ch.wowWrite + 1u) % ch.wowDelay.size();

                // Nonlinear, gain-compensated saturation.
                const float drive = smoothed[2];
                const float inputGain = 1.0f + drive * (2.5f + 12.0f * clamp01(p.distortionTone));
                const float shaped = std::tanh(x * inputGain) / std::tanh(inputGain);
                x += (shaped - x) * drive;

                // Sample-and-hold downsampling + amplitude bit reduction.
                const float digital = smoothed[3];
                const int factor = 1 + static_cast<int>(digital * (2.0f + 23.0f * clamp01(p.digitalRate)));
                if (--ch.holdCounter <= 0) {
                    ch.held = x;
                    ch.holdCounter = factor;
                }
                const float steps = std::pow(2.0f, 15.0f - 11.0f * digital);
                const float crushed = std::round(ch.held * steps) / steps;
                x += (crushed - x) * digital;

                // Three damped feedback combs: light compact stereo ambience.
                float reflections = 0.0f;
                for (size_t k = 0; k < 3; ++k) {
                    auto& comb = ch.reverb[k];
                    const size_t idx = ch.revWrite[k];
                    const float delayed = comb[idx];
                    ch.revDamped[k] = ch.revDamped[k] * 0.43f + delayed * 0.57f;
                    comb[idx] = std::clamp(x + ch.revDamped[k] *
                        (0.43f + 0.37f * clamp01(p.spaceSize)), -4.0f, 4.0f);
                    reflections += delayed;
                    ch.revWrite[k] = (idx + 1u) % comb.size();
                }
                x = x * (1.0f - 0.28f * smoothed[4]) + reflections * (0.18f * smoothed[4]);

                // Magnetic wear: gently smoothed stochastic dropouts.
                if (--ch.dropCounter <= 0) {
                    const float decision = random01();
                    ch.dropTarget = (decision > 0.60f)
                        ? 1.0f - smoothed[5] * (0.15f + 0.78f * random01())
                        : 1.0f;
                    ch.dropCounter = std::max(1, static_cast<int>(
                        rate * (0.03f + random01() * (0.10f + 0.40f * (1.0f - clamp01(p.magneticSpeed))))));
                }
                ch.dropLevel += (ch.dropTarget - ch.dropLevel) * std::min(1.0f, 38.0f / rate);
                x *= ch.dropLevel;

                // End-of-chain focus filters; zero parameter really bypasses.
                if (p.lowCut > 0.00001f) {
                    ch.hpMemory = hpA * ch.hpMemory + (1.0f - hpA) * x;
                    x -= ch.hpMemory;
                } else {
                    ch.hpMemory = x;
                }
                if (p.highCut > 0.00001f) {
                    ch.lpMemory = lpA * ch.lpMemory + (1.0f - lpA) * x;
                    x = ch.lpMemory;
                } else {
                    ch.lpMemory = x;
                }

                const float result = (dry + (x - dry) * mix) * outputGain;
                data[c][i] = std::isfinite(result) ? result : 0.0f;
            }
        }
    }

private:
    struct Channel {
        std::vector<float> wowDelay;
        size_t wowWrite = 0;
        std::array<std::vector<float>, 3> reverb;
        std::array<size_t, 3> revWrite {{ 0, 0, 0 }};
        std::array<float, 3> revDamped {{ 0.0f, 0.0f, 0.0f }};
        float noiseLP = 0.0f, hpMemory = 0.0f, lpMemory = 0.0f;
        float held = 0.0f;
        int holdCounter = 0, dropCounter = 0;
        float dropTarget = 1.0f, dropLevel = 1.0f;
    };

    float random01() noexcept {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        return static_cast<float>(rng & 0x00ffffffu) / 16777216.0f;
    }
    float randomSigned() noexcept { return random01() * 2.0f - 1.0f; }

    float rate = 44100.0f;
    float wowPhase = 0.0f, flutterPhase = 0.0f;
    uint32_t rng = 0x6923a91bu;
    std::array<float, 6> smoothed {};
    std::array<Channel, 2> channels;
};
} // namespace dc23
