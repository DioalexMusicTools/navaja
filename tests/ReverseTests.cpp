#include "ReverseEngine.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

static void check(bool ok, const char* message) {
    if (!ok) { std::cerr << "DioVERB test failed: " << message << "\n"; std::exit(1); }
}
int main() {
    dioverb::Settings p;
    p.sampleRate=48000;
    p.bpm=120;
    check(dioverb::lengthInSamples(p)==48000,"half bar at 120bpm -> 1s");
    p.length=dioverb::Length::QuarterBar;
    check(dioverb::lengthInSamples(p)==24000,"quarter bar at 120bpm -> 0.5s");
    p.length=dioverb::Length::OneBar;
    check(dioverb::lengthInSamples(p)==96000,"one bar at 120bpm -> 2s");
    check(dioverb::tremoloCyclesPerBeat(dioverb::Tremolo::Sixteenth)==4.0f,
          "sixteenth tremolo rate");

    const int source=48000;
    std::vector<float> l(source,0),r(source,0);
    // Audible 30 ms piano-like attack after 100ms of silence, with different stereo content.
    for(int i=4800;i<7000;++i) {
        const float envelope=std::exp(-float(i-4800)/900.0f);
        l[size_t(i)]=envelope*0.8f*std::sin(float(i-4800)*0.2f);
        r[size_t(i)]=envelope*0.7f*std::sin(float(i-4800)*0.24f);
    }
    for(double sr : {44100.0,48000.0,96000.0}) {
        p.sampleRate=sr;
        p.bpm=99.0f;
        p.length=dioverb::Length::OneBar;
        p.mode=dioverb::Mode::Vocal;
        p.size=0.8f; p.width=0.8f; p.tone=0.6f;
        p.tremolo=dioverb::Tremolo::Sixteenth;
        p.tremoloDepth=0.6f;
        p.outputDb=3.0f;
        const auto result=dioverb::render(l.data(),r.data(),l.size(),p);
        check(result.valid(),"rendered audio valid");
        check(result.size()==dioverb::lengthInSamples(p),"tempo aligned length");
        float energy=0,peak=0;
        for(size_t i=0;i<result.size();++i) {
            check(std::isfinite(result.left[i]) && std::isfinite(result.right[i]),
                  "all samples finite");
            energy+=std::abs(result.left[i])+std::abs(result.right[i]);
            peak=std::max(peak,std::max(std::abs(result.left[i]),std::abs(result.right[i])));
        }
        check(energy>0.001f,"reverb creates nonzero rising audio");
        check(peak<=0.9f,"safe output ceiling");
        check(std::abs(result.left.front())<0.0001f,"fade in silence");
        check(std::abs(result.left.back())<0.0001f,"fade out silence");
    }
    p.sampleRate=48000;
    p.bpm=120;
    p.length=dioverb::Length::HalfBar;
    p.tremolo=dioverb::Tremolo::Off;
    p.tremoloDepth=0;
    auto withTrem=dioverb::render(l.data(),r.data(),l.size(),p);
    p.tremolo=dioverb::Tremolo::ThirtySecond;
    p.tremoloDepth=1;
    auto withPulse=dioverb::render(l.data(),r.data(),l.size(),p);
    float difference=0;
    for(size_t i=0;i<withTrem.size();++i)
        difference+=std::abs(withTrem.left[i]-withPulse.left[i]);
    check(difference>0.001f,"tremolo changes sound");
    std::fill(l.begin(),l.end(),0);
    std::fill(r.begin(),r.end(),0);
    check(!dioverb::render(l.data(),r.data(),l.size(),p).valid(),"silence rejected");
    std::cout<<"DioVERB: offline reverse reverb, timing and modulation tests passed\n";
}
