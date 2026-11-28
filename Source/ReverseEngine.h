#pragma once
// DioVERB - original portable offline reverse-reverb engine.
// No JUCE dependency: can be tested on any C++17 compiler.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace dioverb {
constexpr float pi = 3.14159265358979323846f;
inline float bounded(float x, float lo, float hi) noexcept {
    return std::clamp(x, lo, hi);
}
enum class Mode { Vocal = 0, Instrument = 1 };
enum class Length { QuarterBar = 0, HalfBar = 1, OneBar = 2 };
enum class Tremolo { Off = 0, Eighth = 1, Sixteenth = 2, ThirtySecond = 3 };
enum class AutoPan { Off = 0, Half = 1, Quarter = 2, Eighth = 3, Sixteenth = 4 };

struct Settings {
    double sampleRate = 48000.0;
    float bpm = 120.0f;
    Mode mode = Mode::Vocal;
    Length length = Length::HalfBar;
    float size = 0.50f;    // 0..1: room geometry
    float decayMs = 2450.0f; // T60 decay time in milliseconds, 250..8000
    float tone = 0.58f;    // 0..1
    float width = 0.80f;   // 0..1
    Tremolo tremolo = Tremolo::Off;
    float tremoloDepth = 0.35f;
    float outputDb = 0.0f;
    // VintageVerb-style editing concepts, original algorithms and independent IDs.
    float reverbMix = 100.0f;       // % wet, 100 preserves v0.2 behavior
    float predelayMs = 20.0f;       // milliseconds
    float dampHighHz = 6000.0f;    // HF damping transition
    float highShelfDb = 0.0f;      // dB (high-frequency reverb tail)
    float bassFreqHz = 300.0f;     // feedback low-frequency transition
    float bassMult = 1.0f;         // LF feedback time ratio, 0.5 to 2x
    float attack = 0.50f;          // rising envelope shape, 0..1
    float earlyDiff = 0.80f;       // allpass diffusion, 0..1
    float lateDiff = 0.85f;        // late/early wet balance, 0..1
    float modRateHz = 0.70f;       // reverb chorus movement in Hz
    float modDepth = 0.0f;         // 0..1; zero preserves older audio
    float highCutHz = 20000.0f;   // wet EQ output
    float lowCutHz = 20.0f;       // wet EQ output
    AutoPan autoPan = AutoPan::Off;
    float panDepth = 0.0f;         // 0..1
};

struct Result {
    std::vector<float> left, right;
    double sampleRate = 48000.0;
    bool valid() const noexcept {
        return !left.empty() && left.size() == right.size();
    }
    size_t size() const noexcept { return left.size(); }
};

inline float lengthInBeats(Length length) noexcept {
    switch(length) {
        case Length::QuarterBar: return 1.0f;
        case Length::HalfBar: return 2.0f;
        case Length::OneBar: return 4.0f;
    }
    return 2.0f;
}
inline size_t lengthInSamples(const Settings& p) noexcept {
    const auto sr = bounded(static_cast<float>(p.sampleRate), 8000.0f, 192000.0f);
    const auto bpm = bounded(p.bpm, 40.0f, 260.0f);
    const auto seconds = 60.0f * lengthInBeats(p.length) / bpm;
    return std::max<size_t>(128, static_cast<size_t>(std::lround(sr * seconds)));
}
inline float autoPanCyclesPerBeat(AutoPan mode) noexcept {
    switch(mode) {
        case AutoPan::Half: return 0.5f;
        case AutoPan::Quarter: return 1.0f;
        case AutoPan::Eighth: return 2.0f;
        case AutoPan::Sixteenth: return 4.0f;
        default: return 0.0f;
    }
}
inline float tremoloCyclesPerBeat(Tremolo t) noexcept {
    switch (t) {
        case Tremolo::Eighth: return 2.0f;
        case Tremolo::Sixteenth: return 4.0f;
        case Tremolo::ThirtySecond: return 8.0f;
        default: return 0.0f;
    }
}

class Delay {
public:
    explicit Delay(size_t frames) : samples(std::max<size_t>(frames, 1), 0.0f) {}
    size_t frameCount() const noexcept { return samples.size(); }
    float readWrite(float input, float feedback,
                    float bassOnePole = 0.0f, float bassFeedback = -1.0f) noexcept {
        const float old = samples[position];
        bassOnePole = bounded(bassOnePole, 0.0f, 0.99999f);
        bassState = bassOnePole * bassState + (1.0f - bassOnePole) * old;
        if (bassFeedback < 0.0f) bassFeedback = feedback;
        const float hi = old - bassState;
        samples[position] = bounded(input + hi * feedback +
                                   bassState * bassFeedback, -4.0f, 4.0f);
        position = (position + 1) % samples.size();
        return old;
    }
    float allpass(float input, float gain = 0.57f) noexcept {
        const float delayed = samples[position];
        const float out = delayed - gain * input;
        samples[position] = bounded(input + gain * out, -4.0f, 4.0f);
        position = (position + 1) % samples.size();
        return out;
    }
    float dampingState = 0.0f;
private:
    float bassState = 0.0f;
    std::vector<float> samples;
    size_t position = 0;
};

class ReverbTank {
public:
    ReverbTank(double rate, int channel, float size, float color, float decayMs,
               const Settings* advanced = nullptr)
        : sampleRate(bounded(static_cast<float>(rate), 8000.0f, 192000.0f)),
          decaySeconds(bounded(decayMs, 250.0f, 8000.0f) * 0.001f),
          damping(advanced
                      ? std::exp(-2.0f * pi * bounded(advanced->dampHighHz,
                                       1000.0f, 16000.0f) / sampleRate)
                      : (0.11f + 0.55f * bounded(color, 0.0f, 1.0f))),
          earlyDiff(advanced ? bounded(advanced->earlyDiff, 0.0f, 1.0f) : 0.8f),
          lateDiff(advanced ? bounded(advanced->lateDiff, 0.0f, 1.0f) : 0.85f),
          highShelfGain(advanced ? std::pow(10.0f,
                         bounded(advanced->highShelfDb, -18.0f, 6.0f) / 20.0f) : 1.0f),
          bassOnePole(advanced ? std::exp(-2.0f * pi *
                         bounded(advanced->bassFreqHz, 60.0f, 1200.0f) / sampleRate) : 0.0f),
          bassMultiplier(advanced ? bounded(advanced->bassMult, 0.5f, 2.0f) : 1.0f),
          sizeForBuffers(bounded(size, 0.0f, 1.0f)),
          combs {
            Delay(count(0.0297f + channel * 0.0009f)),
            Delay(count(0.0371f + channel * 0.0011f)),
            Delay(count(0.0411f + channel * 0.0007f)),
            Delay(count(0.0437f + channel * 0.0014f)),
            Delay(count(0.0539f + channel * 0.0005f)),
            Delay(count(0.0613f + channel * 0.0017f))
          },
          allpasses {
            Delay(count(0.0051f + channel * 0.0003f)),
            Delay(count(0.0017f + channel * 0.0002f))
          }
    {
        // Calibrate feedback with the actual delay length. Size changes the
        // physical spacing/density; Decay independently controls the T60 tail.
        for (size_t i = 0; i < combs.size(); ++i) {
            const float delaySeconds = static_cast<float>(combs[i].frameCount()) / sampleRate;
            feedback[i] = bounded(std::pow(0.001f, delaySeconds / decaySeconds),
                                  0.05f, 0.986f);
            bassFeedback[i] = bounded(
                std::pow(0.001f, delaySeconds / (decaySeconds * bassMultiplier)),
                0.05f, 0.986f);
        }
    }

    float tick(float input) noexcept {
        // Two diffuser allpasses before a wide multi-delay feedback tank.
        const float gain = 0.12f + earlyDiff * 0.67f;
        const float front = allpasses[1].allpass(
            allpasses[0].allpass(input * 0.7f, gain),
            gain * 0.88f);
        float sum = 0.0f;
        for (size_t k = 0; k < combs.size(); ++k) {
            auto& comb = combs[k];
            const float delayed = comb.readWrite(front, feedback[k], bassOnePole, bassFeedback[k]);
            comb.dampingState = (1.0f - damping) * delayed + damping * comb.dampingState;
            sum += comb.dampingState;
        }
        const float lateWet = sum * (1.0f / float(combs.size()));
        shelfState += 0.12f * (lateWet - shelfState);
        const float colored = shelfState + (lateWet - shelfState) * highShelfGain;
        return colored * (0.3f + 0.7f * lateDiff)
             + front * (1.0f - lateDiff) * 0.28f;
    }

private:
    size_t count(float seconds) const {
        return std::max<size_t>(1, static_cast<size_t>(sampleRate * seconds *
              (0.70f + 0.65f * sizeForBuffers)));
    }
    // Size of all delay lines chosen once during construction.
    const float sampleRate, decaySeconds, damping;
    const float earlyDiff, lateDiff, highShelfGain, bassOnePole, bassMultiplier;
    const float sizeForBuffers;
    float shelfState = 0.0f;
    std::array<float, 6> feedback {{0,0,0,0,0,0}};
    std::array<float, 6> bassFeedback {{0,0,0,0,0,0}};
    std::array<Delay, 6> combs;
    std::array<Delay, 2> allpasses;
};

// The first meaningful sound in the captured clip is used as the landing event.
// Render: reverse source -> diffuse wet tail -> reverse tail -> beat-sized WAV.
// This creates a rising *pre*-verb rather than a realtime reverb on the insert.
inline Result render(const float* inputL, const float* inputR,
                     size_t capturedFrames, const Settings& settings) {
    Result result;
    const float sr = bounded(static_cast<float>(settings.sampleRate),
                             8000.0f, 192000.0f);
    result.sampleRate = sr;
    if (!inputL || !inputR || capturedFrames == 0) return result;

    float peak = 0.0f;
    for (size_t i = 0; i < capturedFrames; ++i)
        peak = std::max(peak, std::max(std::abs(inputL[i]), std::abs(inputR[i])));
    if (!std::isfinite(peak) || peak < 0.002f) return result;

    const float threshold = std::max(0.002f, peak * 0.14f);
    size_t onset = 0;
    while (onset < capturedFrames &&
           std::max(std::abs(inputL[onset]), std::abs(inputR[onset])) < threshold)
        ++onset;
    if (onset >= capturedFrames) return result;

    const auto preroll = static_cast<size_t>(sr * 0.012f);
    const size_t begin = onset > preroll ? onset - preroll : 0;
    const size_t sourceFrames = std::min<size_t>(
        capturedFrames - begin,
        static_cast<size_t>(sr * (settings.mode == Mode::Vocal ? 1.15f : 0.90f)));
    if (!sourceFrames) return result;

    const size_t tailFrames = lengthInSamples(settings);
    result.left.assign(tailFrames, 0.0f);
    result.right.assign(tailFrames, 0.0f);

    const float size = bounded(settings.size, 0.0f, 1.0f);
    const float color = bounded(settings.tone, 0.0f, 1.0f);
    ReverbTank tanks[2] = {
        ReverbTank(sr, 0, size, 1.0f - color, settings.decayMs, &settings),
        ReverbTank(sr, 1, size, 1.0f - color, settings.decayMs, &settings)
    };
    // Offline, bounded predelay and modulated wet delays; never run in processBlock.
    const size_t predelay = static_cast<size_t>(
        sr * bounded(settings.predelayMs, 0.0f, 250.0f) * 0.001f);
    const size_t modCount = static_cast<size_t>(sr * 0.040f) + 32u;
    std::array<std::vector<float>, 2> modBuffer {
        std::vector<float>(modCount, 0.0f),
        std::vector<float>(modCount, 0.0f)
    };
    size_t modWrite = 0;
    float eqHpState[2]{}, eqLpState[2]{};
    const float eqLo = bounded(settings.lowCutHz, 20.0f, std::min(2500.0f, sr*0.45f));
    const float eqHi = bounded(settings.highCutHz, 1500.0f, std::min(20000.0f, sr*0.45f));
    const float eqHpA = std::exp(-2.0f*pi*eqLo/sr);
    const float eqLpA = std::exp(-2.0f*pi*eqHi/sr);
    const float modDepth = bounded(settings.modDepth, 0.0f, 1.0f);
    const float modRate = bounded(settings.modRateHz, 0.10f, 8.0f);
    const float lowHz = 90.0f + (1.0f - color) * 130.0f;
    const float topHz = 2000.0f + color * 12500.0f;
    const float hiA = std::exp(-2.0f * pi * lowHz / sr);
    const float lpA = 1.0f - std::exp(-2.0f * pi * topHz / sr);
    float hpState[2]{0, 0}, lpState[2]{0, 0};
    // Soft excitation keeps the wet tail from producing unpleasant clipped peaks.
    const float excitation = settings.mode == Mode::Vocal ? 0.72f : 0.83f;

    // Only store the naturally decaying reverb *after* the reverse input stops.
    for (size_t frame = 0; frame < sourceFrames + tailFrames; ++frame) {
        float wet[2]{};
        // Predelay shifts the reverb excitation relative to the fixed render window.
        const size_t sourceIndex = frame >= predelay ? frame - predelay : sourceFrames;
        const float dryL = sourceIndex < sourceFrames
            ? inputL[begin + sourceFrames - sourceIndex - 1] : 0.0f;
        const float dryR = sourceIndex < sourceFrames
            ? inputR[begin + sourceFrames - sourceIndex - 1] : 0.0f;
        for (int c = 0; c < 2; ++c) {
            const float x = (c == 0 ? dryL : dryR) * excitation;
            float y = tanks[c].tick(std::tanh(x * 1.3f));
            hpState[c] = hiA * hpState[c] + (1.0f - hiA) * y;
            y -= hpState[c];
            lpState[c] += lpA * (y - lpState[c]);
            // Chorus-style time modulation on the wet reflection field.
            float colored = lpState[c];
            if (modDepth > 0.0001f) {
                const float cycle = 2.0f*pi*modRate*float(frame)/sr
                                  + (c == 0 ? 0.0f : pi*0.5f);
                const float delayInSamples = sr*(0.007f + modDepth*0.004f*std::sin(cycle));
                float read = float(modWrite)-delayInSamples;
                while (read < 0.0f) read += float(modCount);
                const auto a = static_cast<size_t>(read);
                const auto b = (a+1u) % modCount;
                const float frac = read - float(a);
                const float delayed = modBuffer[size_t(c)][a]*(1.0f-frac)
                                    + modBuffer[size_t(c)][b]*frac;
                colored += (delayed - colored) * modDepth * 0.55f;
            }
            modBuffer[size_t(c)][modWrite] = lpState[c];
            if (settings.lowCutHz > 20.0f) {
                eqHpState[c] = eqHpA * eqHpState[c] + (1.0f-eqHpA) * colored;
                colored -= eqHpState[c];
            }
            if (settings.highCutHz < 19999.0f) {
                eqLpState[c] = eqLpA * eqLpState[c] + (1.0f-eqLpA)*colored;
                colored = eqLpState[c];
            }
            wet[c] = colored;
        }
        modWrite = (modWrite + 1u) % modCount;
        if (frame >= sourceFrames) {
            const size_t out = tailFrames - 1 - (frame - sourceFrames);
            const float mid = 0.5f * (wet[0] + wet[1]);
            const float side = 0.5f * (wet[0] - wet[1]) *
                               (0.15f + bounded(settings.width, 0.0f, 1.0f) * 1.75f);
            result.left[out] = mid + side;
            result.right[out] = mid - side;
        }
    }

    // Gain and musical tremolo are applied after reversing the tail.
    const float cycles = tremoloCyclesPerBeat(settings.tremolo);
    const float beatHz = bounded(settings.bpm, 40.0f, 260.0f) / 60.0f;
    const float depth = bounded(settings.tremoloDepth, 0.0f, 1.0f);
    const float gain = std::pow(10.0f, bounded(settings.outputDb, -18.0f, 12.0f) / 20.0f);
    // Fixed 5 ms fade-in and 3 ms fade-out to remove edge clicks.
    const size_t fadeIn = std::max<size_t>(1, static_cast<size_t>(0.005f * sr));
    const size_t fadeOut = std::max<size_t>(1, static_cast<size_t>(0.003f * sr));
    float maxAbs = 0.0f;
    for (size_t i = 0; i < tailFrames; ++i) {
        const float u = float(i) / float(tailFrames - 1);
        const float attackExponent = 0.22f + 2.2f * bounded(settings.attack, 0.0f, 1.0f);
        const float rise = std::pow(u, attackExponent);
        const float inFade = std::min(1.0f, float(i) / float(fadeIn));
        const float outFade = std::min(1.0f, float(tailFrames - i - 1) / float(fadeOut));
        float tremGain = 1.0f;
        if (cycles > 0.0f)
            tremGain = 1.0f - depth * (0.5f - 0.5f *
                std::cos(2.0f * pi * beatHz * cycles * (float(i) / sr)));
        const float panCycles = autoPanCyclesPerBeat(settings.autoPan);
        const float pan = panCycles > 0.0f
            ? bounded(settings.panDepth,0.0f,1.0f) *
              std::sin(2.0f*pi*beatHz*panCycles*(float(i)/sr)) : 0.0f;
        // Equal-power stereo movement, independent of Tremolo gain.
        const float panLeft = std::sqrt(std::max(0.0f,1.0f-pan));
        const float panRight = std::sqrt(std::max(0.0f,1.0f+pan));
        const float envelope = gain * rise * inFade * outFade * tremGain;
        const float mix = bounded(settings.reverbMix, 0.0f, 100.0f) * 0.01f;
        // Optional dry reverse phrase: aligned so its onset ends at the landing point.
        const size_t reverseIndex = tailFrames - 1 - i;
        const float dryReverseL = reverseIndex < sourceFrames
            ? inputL[begin + reverseIndex] * 0.5f : 0.0f;
        const float dryReverseR = reverseIndex < sourceFrames
            ? inputR[begin + reverseIndex] * 0.5f : 0.0f;
        result.left[i] = (result.left[i] * mix * panLeft
                         + dryReverseL * (1.0f - mix)) * envelope;
        result.right[i] = (result.right[i] * mix * panRight
                          + dryReverseR * (1.0f - mix)) * envelope;
        maxAbs = std::max(maxAbs, std::max(std::abs(result.left[i]), std::abs(result.right[i])));
    }
    // Avoid overflows from loud source recordings; respect output gain until -1 dBFS.
    if (maxAbs > 0.89125f) {
        const float safe = 0.89125f / maxAbs;
        for (size_t i = 0; i < tailFrames; ++i) {
            result.left[i] *= safe;
            result.right[i] *= safe;
        }
    }
    return result;
}
} // namespace dioverb
