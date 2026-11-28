#pragma once
#include <array>
#include <cstddef>
#include <algorithm>

namespace dc23 {
struct FactoryPreset {
    const char* name = "CLEAN SLATE";
    const char* category = "UTILITY";
    std::array<float, 6> amount {{0, 0, 0, 0, 0, 0}};
    std::array<float, 6> flux {{0, 0, 0, 0, 0, 0}};
    int noiseStyle = 0;
    int distortionStyle = 0;
    int division = 3;                  // 1/8 at 120 BPM
    bool echoOn = false;
    bool lowCutOn = false, highCutOn = false;
    float echoMix = 0.0f;             // 0% dry / 100% wet
    float feedback = 35.0f;           // percent
    float lowCutHz = 62.0f, highCutHz = 8778.0f;
    float decayMs = 50.0f, wearMs = 50.0f;
    float magnitude = 100.0f, mix = 50.0f, width = 100.0f;
    float noiseColor = 35.0f, wobbleHz = 12.0f, driveTone = 4.2f;
    float digitalRate = 25.0f, digitalSmooth = 55.0f;
};

inline const std::array<FactoryPreset, 20>& factoryPresets() {
    using F = FactoryPreset;
    using A = std::array<float, 6>;
    static const auto presets = [] {
        auto make = [](const char* name, const char* cat, A amount, A flux,
                       int noise, int dist, float decayMs, float wearMs,
                       float lowHz, float highHz, bool echo, int division,
                       float echoMix, float wobbleHz, float width) {
            F p;
            p.name = name; p.category = cat;
            p.amount = amount; p.flux = flux;
            p.noiseStyle = noise; p.distortionStyle = dist;
            // Port original preset intentions into the new public v1 ranges.
            p.decayMs = std::clamp(decayMs / 40.0f, 0.0f, 100.0f);
            p.wearMs = std::clamp(wearMs / 7.5f, 0.0f, 100.0f);
            p.lowCutHz = lowHz; p.highCutHz = highHz;
            p.lowCutOn = lowHz > 20.0f; p.highCutOn = highHz < 20000.0f;
            p.echoOn = echo; p.division = division;
            p.echoMix = echo ? std::clamp((echoMix + 100.0f) * 0.5f, 0.0f, 100.0f) : 0.0f;
            p.wobbleHz = std::clamp(6.0f + wobbleHz * 12.0f, 6.0f, 30.0f);
            p.width = std::clamp(width * 2.0f, 0.0f, 200.0f);
            return p;
        };
        // Original designs by DC-23. No copied factory preset values or names.
        std::array<F, 20> list {{
            make("CLEAN SLATE",       "UTILITY", A{0,0,0,0,0,0},       A{0,0,0,0,0,0},      0,0,1200,310,20,20000,false,3,0,0.9f,50),
            make("CASSETTE WARMTH",   "TAPE",    A{12,18,34,3,8,8},    A{18,19,10,4,6,19},    0,0,1100,320,55,15500,false,3,0,0.6f,49),
            make("NIGHT BUS",        "TAPE",    A{23,38,18,5,16,25},  A{24,32,10,5,20,50},   0,0,1700,440,120,10200,true,3,-16,0.47f,59),
            make("DUSTY PIANO",      "KEYS",    A{34,20,21,9,17,16},  A{32,18,9,18,6,24},    1,0,1450,360,105,12200,false,3,0,0.76f,48),
            make("VINYL MORNING",    "VINYL",   A{42,9,13,1,6,9},     A{15,8,5,0,0,28},      1,0,800,420,50,16800,false,3,0,0.9f,48),
            make("BEAT TAPE",        "DRUMS",   A{19,23,44,17,6,19},  A{12,12,15,15,6,32},   0,1,700,240,62,15600,false,5,0,1.1f,51),
            make("WORN DRUM LOOP",   "DRUMS",   A{20,39,56,30,4,48},  A{28,43,25,24,4,73},   0,1,480,130,125,8500,false,3,0,1.22f,47),
            make("BIT CRUMBLE",      "DIGITAL", A{3,14,27,85,0,26},   A{5,9,17,35,0,52},     2,2,500,190,220,7200,false,3,0,0.9f,38),
            make("DREAMY DRIFT",     "AMBIENT", A{16,56,12,4,53,29},  A{25,65,6,4,41,58},    0,0,2850,490,110,13500,true,2,10,0.33f,70),
            make("SOFT ORGAN",       "KEYS",    A{8,8,29,0,23,4},     A{10,8,10,0,8,12},     0,1,1750,600,58,18000,true,5,-36,0.5f,54),
            make("SPACIOUS PAD",     "AMBIENT", A{10,37,8,0,72,16},   A{15,54,4,0,54,35},    0,0,3400,400,78,14700,true,0,18,0.27f,76),
            make("GLASSY REPEATS",   "ECHO",    A{3,6,6,0,18,0},      A{3,4,4,0,7,0},        0,0,1900,540,20,20000,true,3,45,1.1f,68),
            make("NEON STAIRS",      "ECHO",    A{9,15,23,12,36,10},  A{8,17,9,15,18,14},    0,0,2100,365,89,16500,true,5,15,0.85f,65),
            make("LOST TRANSMISSION","RADIO",   A{45,34,66,57,5,56},  A{65,54,45,60,0,83},   2,2,600,140,340,5200,true,4,-31,1.6f,35),
            make("OLD RADIO",        "RADIO",   A{39,18,49,49,3,23},  A{26,15,20,18,0,41},   2,1,550,220,300,4600,false,3,0,0.82f,21),
            make("MONO MEMORY",      "VINYL",   A{22,24,18,8,35,25},  A{29,33,10,8,31,50},   1,0,2400,310,170,9500,true,3,-27,0.45f,0),
            make("ANALOG BUS",       "MASTER",  A{4,3,22,0,0,0},      A{3,3,5,0,0,0},        0,0,900,550,20,20000,false,3,0,0.5f,50),
            make("SATURATED 808",    "BASS",    A{1,2,55,9,0,2},      A{1,0,17,3,0,0},       0,1,600,620,20,20000,false,3,0,0.2f,51),
            make("TAPE FLUTTER",     "TAPE",    A{18,71,10,3,12,42},  A{19,79,6,4,22,66},    0,0,1300,180,55,12500,false,3,0,1.9f,51),
            make("SLUMBER KEYS",     "KEYS",    A{32,28,19,7,58,20},  A{39,33,8,7,44,36},    1,0,3000,370,105,11000,true,2,-6,0.41f,67)
        }};
        for (auto& p : list) {
            p.noiseColor = (p.noiseStyle == 2 ? 45.0f : 38.0f);
            p.driveTone = (p.distortionStyle == 2 ? 4.2f : -1.0f);
            p.digitalRate = (p.amount[3] > 40 ? 38.5f : 22.5f);
            p.digitalSmooth = (p.amount[3] > 40 ? 18.0f : 62.0f);
            p.feedback = (p.echoOn ? 42.0f : 34.0f);
        }
        return list;
    }();
    return presets;
}
} // namespace dc23
