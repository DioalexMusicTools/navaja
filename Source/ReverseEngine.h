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

struct Settings {
    double sampleRate = 48000.0;
    float bpm = 120.0f;
    Mode mode = Mode::Vocal;
    Length length = Length::HalfBar;
    float size = 0.50f;    // 0..1
    float tone = 0.58f;    // 0..1
    float width = 0.80f;   // 0..1
    Tremolo tremolo = Tremolo::Off;
    float tremoloDepth = 0.35f;
    float outputDb = 0.0f;
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
    float readWrite(float input, float feedback) noexcept {
        const float old = samples[position];
        samples[position] = bounded(input + old * feedback, -4.0f, 4.0f);
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
    std::vector<float> samples;
    size_t position = 0;
};

class ReverbTank {
public:
    ReverbTank(double rate, int channel, float size, float color)
        : sampleRate(bounded(static_cast<float>(rate), 8000.0f, 192000.0f)),
          decaySeconds(0.6f + 3.2f * bounded(size, 0.0f, 1.0f)),
          damping(0.11f + 0.55f * bounded(color, 0.0f, 1.0f)),
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
        constexpr float original[] = {0.0297f, 0.0371f, 0.0411f, 0.0437f, 0.0539f, 0.0613f};
        for (size_t i = 0; i < combs.size(); ++i) {
            const float stretch = 0.7f + bounded(size, 0.0f, 1.0f) * 0.65f;
            const float sec = original[i] * stretch + channel * 0.0005f;
            feedback[i] = bounded(std::pow(0.001f, sec / decaySeconds), 0.05f, 0.94f);
        }
    }

    float tick(float input) noexcept {
        // Two diffuser allpasses before a wide multi-delay feedback tank.
        const float front = allpasses[1].allpass(allpasses[0].allpass(input * 0.7f));
        float sum = 0.0f;
        for (size_t k = 0; k < combs.size(); ++k) {
            auto& comb = combs[k];
            const float delayed = comb.readWrite(front, feedback[k]);
            comb.dampingState = (1.0f - damping) * delayed + damping * comb.dampingState;
            sum += comb.dampingState;
        }
        return sum * (1.0f / float(combs.size()));
    }

private:
    size_t count(float seconds) const {
        return std::max<size_t>(1, static_cast<size_t>(sampleRate * seconds *
              (0.70f + 0.65f * sizeForBuffers)));
    }
    // Size of all delay lines chosen once during construction.
    const float sampleRate, decaySeconds, damping;
    const float sizeForBuffers = 0.65f;
    std::array<float, 6> feedback {{0,0,0,0,0,0}};
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
    ReverbTank tanks[2] = {ReverbTank(sr, 0, size, 1.0f - color),
                            ReverbTank(sr, 1, size, 1.0f - color)};
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
        const float dryL = frame < sourceFrames
            ? inputL[begin + sourceFrames - frame - 1] : 0.0f;
        const float dryR = frame < sourceFrames
            ? inputR[begin + sourceFrames - frame - 1] : 0.0f;
        for (int c = 0; c < 2; ++c) {
            const float x = (c == 0 ? dryL : dryR) * excitation;
            float y = tanks[c].tick(std::tanh(x * 1.3f));
            hpState[c] = hiA * hpState[c] + (1.0f - hiA) * y;
            y -= hpState[c];
            lpState[c] += lpA * (y - lpState[c]);
            wet[c] = lpState[c];
        }
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
        const float rise = std::pow(u, settings.mode == Mode::Vocal ? 0.63f : 0.74f);
        const float inFade = std::min(1.0f, float(i) / float(fadeIn));
        const float outFade = std::min(1.0f, float(tailFrames - i - 1) / float(fadeOut));
        float tremGain = 1.0f;
        if (cycles > 0.0f)
            tremGain = 1.0f - depth * (0.5f - 0.5f *
                std::cos(2.0f * pi * beatHz * cycles * (float(i) / sr)));
        const float envelope = gain * rise * inFade * outFade * tremGain;
        result.left[i] *= envelope;
        result.right[i] *= envelope;
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
