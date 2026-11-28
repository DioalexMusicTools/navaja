#pragma once
// DC-23 DSP v1.0: original, portable, allocation-free on the audio thread.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace dc23 {
constexpr float pi = 3.14159265358979323846f;
inline float limit(float v) noexcept { return std::clamp(v, 0.0f, 1.0f); }

struct Parameters {
    std::array<float, 6> amount {{0.15f, 0.20f, 0.15f, 0.0f, 0.10f, 0.10f}};
    std::array<bool, 6> enabled {{true, true, true, true, true, true}};
    std::array<float, 6> flux {{0.12f, 0.10f, 0.07f, 0.08f, 0.05f, 0.30f}};
    float magnitude = 1.0f;
    float mix = 1.0f;
    float outputDb = 5.0f;
    float noiseColor = 0.35f;
    int noiseStyle = 0;             // 0 tape, 1 vinyl, 2 mains hum
    float wobbleRate = 12.0f;
    float distortionTone = 4.2f; // dB signed tonal shelf
    int distortionStyle = 0;       // 0 tape, 1 valve, 2 fuzz
    float digitalRate = 0.5f; // normalized from 0–50% Crush
    float digitalSmooth = 0.55f;
    float spaceDecayMs = 50.0f;
    float magneticIntervalMs = 50.0f;
    float lowCutHz = 0.0f;
    float highCutHz = 0.0f;
    float width = 1.0f;            // 0 mono, 1 unmodified, 2 extra wide
    float echoMix = 0.0f;
    float echoTimeMs = 290.0f;
    float echoFeedback = 0.35f;
};

class Engine {
public:
    void prepare(double sampleRate) {
        rate = std::max(8000.0f, static_cast<float>(sampleRate));
        for (int c = 0; c != 2; ++c) {
            auto& ch = channels[static_cast<size_t>(c)];
            ch.wow.assign(static_cast<size_t>(rate * 0.055f) + 16u, 0.0f);
            ch.echo.assign(static_cast<size_t>(rate * 4.1f) + 16u, 0.0f);
            constexpr float combTimes[] = {0.0297f, 0.0371f, 0.0411f, 0.0437f};
            for (size_t k = 0; k != 4; ++k)
                ch.combs[k].assign(static_cast<size_t>(rate * (combTimes[k] + c * 0.0011f)) + 1u, 0.0f);
            ch.allpass[0].assign(static_cast<size_t>(rate * (0.005f + c * 0.0004f)) + 1u, 0.0f);
            ch.allpass[1].assign(static_cast<size_t>(rate * (0.0017f + c * 0.0002f)) + 1u, 0.0f);
        }
        reset();
    }

    void reset() noexcept {
        for (auto& ch : channels) {
            std::fill(ch.wow.begin(), ch.wow.end(), 0.0f);
            std::fill(ch.echo.begin(), ch.echo.end(), 0.0f);
            for (auto& buf : ch.combs) std::fill(buf.begin(), buf.end(), 0.0f);
            for (auto& buf : ch.allpass) std::fill(buf.begin(), buf.end(), 0.0f);
            ch.wowWrite = ch.echoWrite = 0;
            ch.combPos.fill(0); ch.allpassPos.fill(0);
            ch.combDamp.fill(0.0f);
            ch.noiseLP = ch.crackle = ch.humPhase = ch.toneMemory = 0.0f;
            ch.digitalLP = ch.hp = ch.lp = ch.echoLP = 0.0f;
            ch.held = 0.0f; ch.holdSamples = 0;
            ch.dropSamples = 0; ch.drop = ch.dropTarget = 1.0f;
        }
        smoothAmount.fill(0.0f); fluxTarget.fill(0.0f); fluxDrift.fill(0.0f);
        fluxSamples = 0;
        wowPhase = flutterPhase = 0.0f;
        smoothMix = 0.0f; smoothEchoMix = 0.0f;
        smoothGain = 1.0f; smoothWidth = 1.0f;
        smoothEchoDelay = rate * 0.29f;
        rng = 0x6923a91bu;
    }

    void process(float* const* data, int channelsUsed, int samples, const Parameters& p) noexcept {
        if (!data || samples <= 0 || channelsUsed <= 0) return;
        channelsUsed = std::clamp(channelsUsed, 0, 2);
        if (channels[0].wow.empty()) return;

        const float fastSlew = std::min(1.0f, 1.0f / (0.015f * rate));
        const float slowSlew = std::min(1.0f, 1.0f / (0.2f * rate));
        const float mixGoal = limit(p.mix);
        const float echoGoal = limit(p.echoMix) * limit(p.magnitude);
        const float gainGoal = std::pow(10.0f, std::clamp(p.outputDb, -18.0f, 18.0f) / 20.0f);
        const float widthGoal = std::clamp(p.width, 0.0f, 2.0f);
        const float echoDelayGoal = rate * std::clamp(p.echoTimeMs, 50.0f, 4000.0f) / 1000.0f;
        const float echoFeedback = std::clamp(p.echoFeedback, 0.0f, 0.8f);
        const float hpHz = std::clamp(p.lowCutHz, 20.0f, 20000.0f);
        const float lpHz = std::clamp(p.highCutHz, 20.0f, 20000.0f);
        const float hpA = std::exp(-2.0f * pi * std::min(hpHz, 0.45f * rate) / rate);
        const float lpA = std::exp(-2.0f * pi * std::min(lpHz, 0.45f * rate) / rate);
        const float wobbleRate = std::clamp(p.wobbleRate, 6.0f, 30.0f);
        // T60 calibration for each comb delay: a repeat drops to -60dB in decayMs.
        std::array<std::array<float, 4>, 2> combFeedback {};
        const float reverbT60 = std::clamp(p.spaceDecayMs, 0.0f, 100.0f) * 0.001f;
        for (int c = 0; c < channelsUsed; ++c)
            for (size_t k = 0; k < 4; ++k) {
                const float delaySec = float(channels[size_t(c)].combs[k].size()) / rate;
                combFeedback[size_t(c)][k] = reverbT60 > 0.0f
                    ? std::pow(0.001f, delaySec / reverbT60) : 0.0f;
            }
        const int noiseStyle = std::clamp(p.noiseStyle, 0, 2);
        const int distortionStyle = std::clamp(p.distortionStyle, 0, 2);

        std::array<float, 6> targets {};
        for (size_t n = 0; n < targets.size(); ++n)
            targets[n] = limit(p.amount[n]) * (p.enabled[n] ? 1.0f : 0.0f) * limit(p.magnitude);

        for (int i = 0; i < samples; ++i) {
            if (--fluxSamples <= 0) {
                for (float& target : fluxTarget) target = randomSigned();
                fluxSamples = std::max(1, static_cast<int>(rate * 0.13f));
            }
            std::array<float, 6> depth {};
            for (size_t n = 0; n < 6; ++n) {
                fluxDrift[n] += (fluxTarget[n] - fluxDrift[n]) * slowSlew;
                const float animated = targets[n] * (1.0f + 0.6f * limit(p.flux[n]) * fluxDrift[n]);
                smoothAmount[n] += (limit(animated) - smoothAmount[n]) * fastSlew;
                depth[n] = smoothAmount[n];
            }
            smoothMix += (mixGoal - smoothMix) * fastSlew;
            smoothEchoMix += (echoGoal - smoothEchoMix) * fastSlew;
            smoothGain += (gainGoal - smoothGain) * fastSlew;
            smoothWidth += (widthGoal - smoothWidth) * fastSlew;
            smoothEchoDelay += (echoDelayGoal - smoothEchoDelay) * fastSlew;

            const float slowLfo = std::sin(wowPhase);
            const float flutter = std::sin(flutterPhase);
            wowPhase += 2.0f * pi * wobbleRate / rate;
            flutterPhase += 2.0f * pi * (9.4f + wobbleRate * 0.3f) / rate;
            if (wowPhase >= 2.0f * pi) wowPhase -= 2.0f * pi;
            if (flutterPhase >= 2.0f * pi) flutterPhase -= 2.0f * pi;

            float wet[2] {0.0f, 0.0f};
            float dry[2] {0.0f, 0.0f};
            for (int c = 0; c < channelsUsed; ++c) {
                auto& ch = channels[static_cast<size_t>(c)];
                dry[c] = data[c][i];
                float x = dry[c];

                // Noise models: tape hiss, intermittent vinyl crackles or electronic hum.
                const float white = randomSigned();
                ch.noiseLP += 0.035f * (white - ch.noiseLP);
                const float tone = limit(p.noiseColor);
                const float hiss = (1.0f - tone) * white + tone * 3.0f * ch.noiseLP;
                float texture = hiss;
                if (noiseStyle == 1) {
                    if (random01() < 0.00009f) ch.crackle = (randomSigned() * 0.9f);
                    ch.crackle *= 0.91f;
                    texture = 0.15f * hiss + ch.crackle;
                } else if (noiseStyle == 2) {
                    ch.humPhase += 2.0f * pi * 60.0f / rate;
                    if (ch.humPhase >= 2.0f * pi) ch.humPhase -= 2.0f * pi;
                    texture = 0.75f * std::sin(ch.humPhase) +
                              0.25f * std::sin(ch.humPhase * 2.0f) + hiss * 0.08f;
                }
                x += texture * 0.095f * depth[0] * depth[0];

                // Tape wow/flutter: fractional delay and smooth amount transitions.
                ch.wow[ch.wowWrite] = x;
                const float modSeconds = (0.0021f * slowLfo + 0.00023f * flutter) * depth[1];
                const float delaySamples = rate * (0.0065f + modSeconds);
                float read = static_cast<float>(ch.wowWrite) - delaySamples;
                const auto wowSize = static_cast<float>(ch.wow.size());
                while (read < 0.0f) read += wowSize;
                const size_t a = static_cast<size_t>(read);
                const size_t b = (a + 1u) % ch.wow.size();
                const float fraction = read - static_cast<float>(a);
                const float warped = ch.wow[a] * (1.0f - fraction) + ch.wow[b] * fraction;
                x += (warped - x) * depth[1];
                ch.wowWrite = (ch.wowWrite + 1u) % ch.wow.size();

                // Three original distortion curves: tape soft-knee, asymmetric valve, fuzz.
                const float d = depth[2];
                if (d > 0.00001f) {
                    // Signed ±12 dB tilt: boost or cut the bright component
                    // without turning an ostensibly EQ-labelled knob into extra distortion.
                    const float toneDb = std::clamp(p.distortionTone, -12.0f, 12.0f);
                    const float toneGain = std::pow(10.0f, toneDb / 20.0f);
                    ch.toneMemory += (x - ch.toneMemory) * 0.16f;
                    const float pre = ch.toneMemory + (x - ch.toneMemory) * toneGain;
                    const float gain = 1.0f + d * 6.0f;
                    float shaped = 0.0f;
                    if (distortionStyle == 0) {
                        shaped = std::tanh(pre * gain) / std::tanh(gain);
                    } else if (distortionStyle == 1) {
                        const float y = pre * gain + 0.28f;
                        shaped = (std::tanh(y) - std::tanh(0.28f)) / std::tanh(gain);
                    } else {
                        shaped = std::clamp(pre * gain, -0.72f, 0.72f) * (1.0f / 0.72f);
                    }
                    x += (shaped - x) * d;
                } else {
                    ch.toneMemory = x;
                }

                // Sample-and-hold plus variable bit reduction with controllable smoothing.
                const float digital = depth[3];
                const int factor = 1 + static_cast<int>(digital * (2.0f + 29.0f * limit(p.digitalRate)));
                if (--ch.holdSamples <= 0) {
                    ch.held = x;
                    ch.holdSamples = factor;
                }
                const float steps = std::exp2(15.0f - 12.0f * digital);
                const float crushed = std::round(ch.held * steps) / steps;
                const float digitalA = 1.0f - limit(p.digitalSmooth) * 0.82f;
                ch.digitalLP += digitalA * (crushed - ch.digitalLP);
                x += (ch.digitalLP - x) * digital;

                // Diffuse reverb: four damped parallel combs, two all-pass diffusion stages.
                float reflected = 0.0f;
                for (size_t k = 0; k < 4; ++k) {
                    auto& comb = ch.combs[k];
                    const size_t pos = ch.combPos[k];
                    const float previous = comb[pos];
                    ch.combDamp[k] = ch.combDamp[k] * 0.32f + previous * 0.68f;
                    comb[pos] = std::clamp(x + ch.combDamp[k] * combFeedback[size_t(c)][k], -4.0f, 4.0f);
                    reflected += previous;
                    ch.combPos[k] = (pos + 1u) % comb.size();
                }
                reflected *= 0.25f;
                for (size_t k = 0; k < 2; ++k) {
                    auto& ap = ch.allpass[k];
                    const size_t pos = ch.allpassPos[k];
                    const float delayed = ap[pos];
                    const float out = delayed - 0.55f * reflected;
                    ap[pos] = reflected + 0.55f * out;
                    reflected = out;
                    ch.allpassPos[k] = (pos + 1u) % ap.size();
                }
                x = x * (1.0f - 0.12f * depth[4]) + reflected * (0.45f * depth[4]);

                // Magnetic wear: random dropouts with soft attack/recovery.
                if (p.magneticIntervalMs <= 0.0f) {
                    // Zero milliseconds disables new dropouts; recovery remains smooth.
                    ch.dropTarget = 1.0f;
                    ch.dropSamples = 1;
                } else if (--ch.dropSamples <= 0) {
                    ch.dropTarget = random01() > 0.57f
                        ? 1.0f - depth[5] * (0.25f + 0.7f * random01())
                        : 1.0f;
                    ch.dropSamples = std::max(1, static_cast<int>(rate *
                        (0.002f + random01() * std::clamp(p.magneticIntervalMs, 0.0f, 100.0f) * 0.001f)));
                }
                ch.drop += (ch.dropTarget - ch.drop) * std::min(1.0f, 45.0f / rate);
                x *= ch.drop;

                // Echo: rolling, gently filtered tape repeats; original DSP, not RC-20 code.
                const auto echoSize = static_cast<float>(ch.echo.size());
                float echoRead = static_cast<float>(ch.echoWrite) - smoothEchoDelay;
                while (echoRead < 0.0f) echoRead += echoSize;
                const size_t eA = static_cast<size_t>(echoRead);
                const size_t eB = (eA + 1u) % ch.echo.size();
                const float ef = echoRead - static_cast<float>(eA);
                const float echoSample = ch.echo[eA] * (1.0f - ef) + ch.echo[eB] * ef;
                ch.echoLP += 0.25f * (echoSample - ch.echoLP);
                ch.echo[ch.echoWrite] = std::clamp(x + std::tanh(ch.echoLP) * echoFeedback,
                                                   -3.0f, 3.0f);
                ch.echoWrite = (ch.echoWrite + 1u) % ch.echo.size();
                // Bipolar Echo Dry/Wet is a genuine crossfade, not additive echo gain.
                x += (ch.echoLP - x) * smoothEchoMix;

                // Master tone; both filters fully bypassed at zero settings.
                if (p.lowCutHz >= 20.0f) {
                    ch.hp = hpA * ch.hp + (1.0f - hpA) * x;
                    x -= ch.hp;
                } else ch.hp = x;
                if (p.highCutHz >= 20.0f) {
                    ch.lp = lpA * ch.lp + (1.0f - lpA) * x;
                    x = ch.lp;
                } else ch.lp = x;
                wet[c] = x;
            }

            // Mid/side only for stereo; preserve mono signal exactly.
            if (channelsUsed == 2) {
                const float mid = 0.5f * (wet[0] + wet[1]);
                const float side = 0.5f * (wet[0] - wet[1]) * smoothWidth;
                wet[0] = mid + side;
                wet[1] = mid - side;
            }
            for (int c = 0; c < channelsUsed; ++c) {
                const float out = (dry[c] + (wet[c] - dry[c]) * smoothMix) * smoothGain;
                data[c][i] = std::isfinite(out) ? out : 0.0f;
            }
        }
    }

private:
    struct Channel {
        std::vector<float> wow, echo;
        std::array<std::vector<float>, 4> combs;
        std::array<std::vector<float>, 2> allpass;
        size_t wowWrite = 0, echoWrite = 0;
        std::array<size_t, 4> combPos {};
        std::array<size_t, 2> allpassPos {};
        std::array<float, 4> combDamp {};
        float noiseLP = 0.0f, crackle = 0.0f, humPhase = 0.0f;
        float toneMemory = 0.0f, digitalLP = 0.0f;
        float hp = 0.0f, lp = 0.0f, echoLP = 0.0f;
        float held = 0.0f; int holdSamples = 0;
        int dropSamples = 0; float drop = 1.0f, dropTarget = 1.0f;
    };
    float random01() noexcept {
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        return static_cast<float>(rng & 0x00ffffffu) / 16777216.0f;
    }
    float randomSigned() noexcept { return 2.0f * random01() - 1.0f; }

    float rate = 44100.0f;
    float wowPhase = 0.0f, flutterPhase = 0.0f;
    float smoothMix = 0.0f, smoothEchoMix = 0.0f;
    float smoothGain = 1.0f, smoothWidth = 1.0f, smoothEchoDelay = 0.0f;
    std::array<float, 6> smoothAmount {};
    std::array<float, 6> fluxTarget {};
    std::array<float, 6> fluxDrift {};
    int fluxSamples = 0;
    uint32_t rng = 0x6923a91bu;
    std::array<Channel, 2> channels;
};
} // namespace dc23
