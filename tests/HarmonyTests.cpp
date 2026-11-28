#include "Harmony.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>

void require(bool ok,const char* message) {
    if(!ok){std::cerr<<"DioCHORD X test FAILED: "<<message<<"\n";std::exit(1);}
}
int main() {
    const auto& library=diochord::presets();
    require(library.size()==16,"16 original progressions");
    std::set<std::string> names;
    int moods[4]{0,0,0,0};
    for(const auto&p:library) {
        require(names.insert(p.name).second,"unique preset names");
        moods[static_cast<int>(p.mood)]++;
        for(const auto& c:p.chords)
            require(c.degree>=0&&c.degree<7,"diatonic scale degree");
    }
    for(int mood:moods)require(mood==4,"four presets per mood");

    diochord::Settings p;
    for(int preset=0;preset<16;++preset)
      for(int key=0;key<12;++key)
       for(int scale=0;scale<2;++scale)
        for(int feel=0;feel<4;++feel) {
            p.preset=preset;p.tonic=key;p.minor=scale>0;p.feel=feel;
            p.humanize=0.42f;p.strumMs=50;p.bpm=95;
            const auto seq=diochord::makeSequence(p);
            require(seq.count>0&&seq.count<=256,"bounded note count");
            require(std::abs(seq.lengthBeats-16.0f)<0.01f,"4 bars at normal speed");
            float last=-1;
            for(int i=0;i<seq.count;++i){
                const auto& n=seq.notes[static_cast<size_t>(i)];
                require(n.pitch>=24&&n.pitch<=108,"MIDI pitch range");
                require(n.velocity>=1&&n.velocity<=127,"velocity range");
                require(n.startBeat>=0&&n.startBeat<seq.lengthBeats,"event inside loop");
                require(n.durationBeat>0&&n.startBeat+n.durationBeat<=seq.lengthBeats+1e-3f,
                        "note-off fits loop");
                require(n.startBeat>=last,"sorted note events");
                last=n.startBeat;
            }
        }

    p={};p.preset=8;p.tonic=0;p.minor=true;p.humanize=0;p.strumMs=0;
    const auto original=diochord::makeSequence(p);
    require(original.count>0,"base pattern");
    p.tonic=2;
    const auto transposed=diochord::makeSequence(p);
    require(transposed.count==original.count,"transpose preserves note count");
    for(int i=0;i<original.count;++i)
        require(transposed.notes[size_t(i)].pitch-original.notes[size_t(i)].pitch==2,
                "global key transpose +2 semitones");

    p={};p.feel=diochord::Flow;
    const auto whole=diochord::makeSequence(p);
    p.feel=diochord::Stabs;
    const auto stabs=diochord::makeSequence(p);
    p.feel=diochord::Arpeggio;
    const auto arp=diochord::makeSequence(p);
    p.feel=diochord::Bounce332;
    const auto bounce=diochord::makeSequence(p);
    require(stabs.count==whole.count*4,"stabs repeat each chord four times");
    require(arp.count==32,"arpeggio is eight triggers per chord");
    require(bounce.count==whole.count*3,"3-3-2 is three hits");
    p.feel=0;p.speed=0;
    require(diochord::makeSequence(p).lengthBeats==32,"half speed 8 bars");
    p.speed=2;
    require(diochord::makeSequence(p).lengthBeats==8,"double speed 2 bars");

    p={};p.humanize=0;p.strumMs=0;
    const auto mechanical=diochord::makeSequence(p);
    p.humanize=0.9f;p.strumMs=33.0f;p.take=5;
    const auto organic=diochord::makeSequence(p);
    const auto exactRepeat=diochord::makeSequence(p);
    require(organic.count==mechanical.count,"humanization doesn't lose notes");
    float changes=0;
    for(int i=0;i<organic.count;++i) {
        const auto& a=organic.notes[size_t(i)];
        const auto& b=mechanical.notes[size_t(i)];
        changes+=std::abs(a.startBeat-b.startBeat)+
                 std::abs(float(a.velocity-b.velocity))*0.01f;
        require(a.startBeat==exactRepeat.notes[size_t(i)].startBeat&&
                a.velocity==exactRepeat.notes[size_t(i)].velocity,
                "same take reproduces same MIDI");
    }
    require(changes>0.1f,"humanize and strum change the performance");
    p.take=6;
    const auto newTake=diochord::makeSequence(p);
    bool changed=false;
    for(int i=0;i<organic.count;++i)
        if(organic.notes[size_t(i)].velocity!=newTake.notes[size_t(i)].velocity)
            changed=true;
    require(changed,"new take yields a different performance");
    std::cout<<"DioCHORD X harmony, transposition, timing, feel and humanize tests PASSED\n";
    return 0;
}
