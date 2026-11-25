#include "DSP.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

static void require(bool condition, const char* reason) {
    if (!condition) {
        std::cerr << "DC-23 DSP regression failed: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}

int main() {
    dc23::Engine engine;
    engine.prepare(48000.0);
    constexpr int N = 4096;
    std::vector<float> l(N), r(N);
    for (int i = 0; i < N; ++i) {
        l[i] = std::sin(2.0f * dc23::pi * 440.0f * float(i) / 48000.0f) * 0.4f;
        r[i] = -l[i];
    }
    const auto original = l;
    float* ptr[] { l.data(), r.data() };
    dc23::Parameters p;
    p.amount.fill(0.0f);
    p.flux.fill(1.0f);
    p.echoMix = 0.0f;
    engine.process(ptr, 2, N, p);
    for (int i = 0; i < N; ++i)
        require(std::abs(l[i] - original[i]) < 1.0e-6f, "zero-effect transparency");

    engine.reset();
    std::copy(original.begin(), original.end(), l.begin());
    for (int i = 0; i < N; ++i) r[i] = -original[i];
    p.amount.fill(1.0f);
    p.echoMix = 1.0f;
    p.mix = 0.0f;
    engine.process(ptr, 2, N, p);
    for (int i = 0; i < N; ++i)
        require(std::abs(l[i] - original[i]) < 1.0e-6f, "dry mix transparency");

    engine.reset();
    std::fill(l.begin(), l.end(), 0.0f);
    std::fill(r.begin(), r.end(), 0.0f);
    p.amount.fill(0.0f);
    p.amount[0] = 1.0f;
    p.echoMix = 0.0f;
    p.mix = 1.0f;
    engine.process(ptr, 2, N, p);
    float noiseEnergy = 0.0f;
    for (float s : l) noiseEnergy += std::abs(s);
    require(noiseEnergy > 0.001f, "tape noise should be audible");

    // Exercise all 3 noise and distortion models at extreme settings.
    for (int style = 0; style != 3; ++style) {
        engine.reset();
        p.noiseStyle = style;
        p.distortionStyle = style;
        p.amount.fill(1.0f);
        p.echoMix = 1.0f;
        p.echoFeedback = 0.8f;
        p.lowCut = 0.52f;
        p.highCut = 0.71f;
        p.width = 1.8f;
        p.outputDb = 10.0f;
        for (int block = 0; block < 96; ++block) {
            for (int i = 0; i < N; ++i) {
                l[i] = 0.9f * std::sin(float(i + block*N) * 0.11f);
                r[i] = 0.9f * std::cos(float(i + block*N) * 0.17f);
            }
            engine.process(ptr, 2, N, p);
            for (int i = 0; i < N; ++i) {
                require(std::isfinite(l[i]), "left channel finite");
                require(std::isfinite(r[i]), "right channel finite");
            }
        }
    }

    // Input impulse must produce a delayed echo tail.
    engine.reset();
    p = dc23::Parameters{};
    p.amount.fill(0.0f);
    p.echoMix = 1.0f;
    p.echoFeedback = 0.45f;
    p.echoTimeMs = 290.0f;
    p.mix = 1.0f;
    float tailEnergy = 0.0f;
    for (int block = 0; block < 8; ++block) {
        std::fill(l.begin(), l.end(), 0.0f);
        std::fill(r.begin(), r.end(), 0.0f);
        if (block == 0) l[0] = r[0] = 0.9f;
        engine.process(ptr, 2, N, p);
        if (block >= 3) {
            for (float sample : l) tailEnergy += std::abs(sample);
        }
    }
    require(tailEnergy > 0.02f, "echo must produce delayed repeats");

    // Check mono and additional host sample rates.
    for (double rate : {44100.0, 96000.0, 192000.0}) {
        engine.prepare(rate);
        p.amount.fill(1.0f);
        p.echoMix = 0.75f;
        p.flux.fill(1.0f);
        p.outputDb = 12.0f;
        for (int block = 0; block < 16; ++block) {
            for (int i = 0; i < N; ++i)
                l[i] = std::sin(float(i+block*N) * 0.08f);
            engine.process(ptr, 1, N, p);
            for (float sample : l) require(std::isfinite(sample), "mono finite");
        }
    }
    std::cout << "DC-23 0.2 DSP regression suite passed\n";
}
