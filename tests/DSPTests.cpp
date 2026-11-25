#include "DSP.h"
#include "Timing.h"
#include "FactoryPresets.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

static void require(bool condition, const char* reason) {
    if (!condition) {
        std::cerr << "DC-23 0.3 regression failed: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
int main() {
    // Beat duration is derived from the transport, not an arbitrary slider.
    require(std::abs(dc23::syncedEchoMilliseconds(0,120.0f)-1000.0f) < 0.01f,
            "1/2 beat @ 120 = 1000ms");
    require(std::abs(dc23::syncedEchoMilliseconds(1,120.0f)-500.0f) < 0.01f,
            "1/4 beat @ 120 = 500ms");
    require(std::abs(dc23::syncedEchoMilliseconds(3,120.0f)-250.0f) < 0.01f,
            "1/8 beat @ 120 = 250ms");
    require(std::abs(dc23::syncedEchoMilliseconds(5,120.0f)-125.0f) < 0.01f,
            "1/16 beat @ 120 = 125ms");

    const auto& presets = dc23::factoryPresets();
    require(presets.size() >= 18, "expanded factory preset library");
    for (const auto& f : presets) {
        require(f.name && f.category, "named factory preset");
        for (float a : f.amount) require(a >= 0 && a <= 100, "amount % bounds");
        for (float x : f.flux) require(x >= 0 && x <= 100, "flux % bounds");
        require(f.decayMs >= 150 && f.decayMs <= 4000, "decay in ms range");
        require(f.wearMs >= 60 && f.wearMs <= 750, "drop in ms range");
        require(f.echoMix >= -100 && f.echoMix <= 100, "bipolar echo");
    }

    dc23::Engine engine;
    engine.prepare(48000.0);
    constexpr int N=4096;
    std::vector<float> left(N),right(N);
    for (int i=0;i<N;++i) {
        left[i]=std::sin(float(i)*0.21f)*0.4f;
        right[i]=-left[i];
    }
    const auto original=left;
    float* channels[2]={left.data(),right.data()};
    dc23::Parameters p;
    p.amount.fill(0);
    p.mix=1.0f;
    p.echoMix=0;
    engine.process(channels,2,N,p);
    for (int i=0;i<N;++i)
        require(std::abs(left[i]-original[i]) < 1e-6f,"no FX preserves input");

    // -100% bipolar wet = 0 DSP mix. Signal must remain exactly DRY.
    engine.reset();
    std::copy(original.begin(),original.end(),left.begin());
    p.amount.fill(1);
    p.flux.fill(1);
    p.mix=0;
    p.echoMix=1;
    p.lowCutHz=300; p.highCutHz=6000;
    engine.process(channels,2,N,p);
    for (int i=0;i<N;++i)
        require(std::abs(left[i]-original[i]) < 1e-6f,"dry preserves input");

    engine.reset();
    p={};
    p.amount.fill(0);
    p.amount[0]=1.0f;
    p.mix=1;
    std::fill(left.begin(),left.end(),0.0f);
    std::fill(right.begin(),right.end(),0.0f);
    engine.process(channels,2,N,p);
    float noiseEnergy=0;
    for(float x:left)noiseEnergy+=std::abs(x);
    require(noiseEnergy>0.01f,"noise generates audible signal");

    // Echo should provide output after impulse and correctly buffer >1000ms beats.
    engine.reset();
    p={}; p.amount.fill(0); p.mix=1;
    p.echoMix=1; p.echoFeedback=0.4f;
    p.echoTimeMs=dc23::syncedEchoMilliseconds(0,60.0f); // half = 2000 ms
    float delayedEnergy=0;
    for(int block=0;block<28;++block) {
        std::fill(left.begin(),left.end(),0.0f);
        std::fill(right.begin(),right.end(),0.0f);
        if(block==0) left[0]=right[0]=0.8f;
        engine.process(channels,2,N,p);
        if(block>23) for(float x:left)delayedEnergy+=std::abs(x);
    }
    require(delayedEnergy>0.005f,"2-second echo tail");

    // Worst-case stress for noise models, distortions, ms times, filters.
    for(double sr : {44100.0,48000.0,96000.0,192000.0}) {
        engine.prepare(sr);
        for(int style=0;style<3;++style) {
            p={};
            p.noiseStyle=style; p.distortionStyle=style;
            p.amount.fill(1); p.flux.fill(1);
            p.spaceDecayMs=4000; p.magneticIntervalMs=60;
            p.lowCutHz=180; p.highCutHz=6500;
            p.mix=1; p.echoMix=1; p.echoFeedback=0.8f;
            p.width=2; p.outputDb=12;
            for(int block=0;block<12;++block) {
                for(int i=0;i<N;++i) {
                    left[i]=0.9f*std::sin(float(i+block*N)*0.11f);
                    right[i]=0.9f*std::cos(float(i+block*N)*0.17f);
                }
                engine.process(channels,2,N,p);
                for(int i=0;i<N;++i) {
                    require(std::isfinite(left[i]),"left finite");
                    require(std::isfinite(right[i]),"right finite");
                }
            }
            // Mono host support.
            engine.reset();
            for(int i=0;i<N;++i)left[i]=std::sin(float(i)*0.04f);
            engine.process(channels,1,N,p);
            for(float x:left)require(std::isfinite(x),"mono finite");
        }
    }
    std::cout<<"DC-23 0.3 timing, presets, bypass, echo and DSP tests passed\n";
}
