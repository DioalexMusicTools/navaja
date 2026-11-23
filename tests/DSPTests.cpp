#include "DSP.h"
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

void require(bool ok, const char* msg) {
    if (!ok) { std::cerr << "DSP test failure: " << msg << "\n"; std::exit(EXIT_FAILURE); }
}

int main() {
    dc23::Engine engine;
    engine.prepare(48000.0);
    constexpr int count = 4096;
    std::vector<float> l(count), r(count);
    for (int i = 0; i < count; ++i) {
        l[i] = std::sin(2.0f * dc23::pi * 440.0f * float(i) / 48000.0f) * 0.4f;
        r[i] = -l[i];
    }
    const auto original = l;
    float* pointers[2] { l.data(), r.data() };
    dc23::Parameters p;
    p.amount.fill(0.0f);
    p.mix = 1.0f;
    engine.process(pointers, 2, count, p);
    for (int i = 0; i < count; ++i)
        require(std::abs(l[i] - original[i]) < 1.0e-6f, "dry-through exact");

    engine.reset();
    std::copy(original.begin(), original.end(), l.begin());
    for (int i = 0; i < count; ++i) r[i] = -original[i];
    p.amount.fill(1.0f);
    p.mix = 0.0f;
    engine.process(pointers, 2, count, p);
    for (int i = 0; i < count; ++i)
        require(std::abs(l[i] - original[i]) < 1.0e-6f, "dry-through exact");

    engine.reset();
    std::fill(l.begin(), l.end(), 0.0f);
    std::fill(r.begin(), r.end(), 0.0f);
    p.amount.fill(0.0f);
    p.amount[0] = 1.0f;
    p.mix = 1.0f;
    engine.process(pointers, 2, count, p);
    float energy = 0.0f;
    for (float s : l) energy += std::abs(s);
    require(energy > 0.01f, "noise audible");

    engine.reset();
    p.amount.fill(1.0f);
    p.outputDb = 12.0f;
    p.highCut = p.lowCut = 0.5f;
    for (int block = 0; block < 64; ++block) {
        for (int i = 0; i < count; ++i) {
            l[i] = std::sin(float(i + block * count) * 0.083f) * 0.7f;
            r[i] = -l[i];
        }
        engine.process(pointers, 2, count, p);
        for (int i = 0; i < count; ++i) {
            require(std::isfinite(l[i]), "left finite");
            require(std::isfinite(r[i]), "right finite");
        }
    }
    std::cout << "DC-23: DSP smoke tests passed\n";
}
