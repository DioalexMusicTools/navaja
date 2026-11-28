#pragma once
// DioCHORD X: original self-contained harmonic and performance engine.
// Header-only C++17 with no JUCE dependency, for cross-platform tests.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>

namespace diochord {
enum Quality : int { Major, Minor, Maj7, Min7, Dom7, Sus2, Sus4, Dim, Maj9, Min9 };
enum Feel : int { Flow, Stabs, Arpeggio, Bounce332 };
enum Mood : int { Reflective, Uplifting, Urban, Dark };
struct Chord {
    int degree = 0;  // zero-based scale degree
    Quality quality = Major;
};
struct Preset {
    const char* name;
    Mood mood;
    std::array<Chord,4> chords;
};
inline const std::array<Preset,16>& presets() {
    // Original progressions. These are not imported from any commercial plugin.
    static const std::array<Preset,16> library {{
      {"Afterglow",      Reflective, {{{5,Min7}, {3,Maj7}, {0,Maj9}, {4,Min7}}}},
      {"Quiet Streets",  Reflective, {{{0,Min9}, {5,Maj7}, {2,Min7}, {6,Dom7}}}},
      {"Blue Lantern",   Reflective, {{{3,Maj7}, {4,Min7}, {0,Min9}, {5,Maj7}}}},
      {"Last Letter",    Reflective, {{{0,Min7}, {6,Major}, {3,Maj7}, {4,Minor}}}},
      {"Open Skies",     Uplifting, {{{0,Major}, {4,Major}, {5,Minor}, {3,Major}}}},
      {"New Horizon",    Uplifting, {{{3,Maj7}, {0,Major}, {4,Major}, {5,Min7}}}},
      {"Bright Steps",   Uplifting, {{{0,Maj9}, {2,Min7}, {3,Maj7}, {4,Dom7}}}},
      {"Sunrise",        Uplifting, {{{5,Min7}, {3,Major}, {0,Major}, {4,Dom7}}}},
      {"Midnight Bounce",Urban,      {{{0,Min9}, {3,Maj7}, {6,Dom7}, {4,Min7}}}},
      {"Velvet Keys",    Urban,      {{{5,Min9}, {1,Min7}, {4,Dom7}, {0,Maj9}}}},
      {"Sunday Pocket",  Urban,      {{{0,Maj9}, {5,Min7}, {1,Min7}, {4,Dom7}}}},
      {"Golden Loop",    Urban,      {{{2,Min7}, {5,Dom7}, {0,Maj9}, {3,Maj7}}}},
      {"Shadow Signal",  Dark,       {{{0,Min9}, {6,Major}, {5,Minor}, {4,Dom7}}}},
      {"Nocturne",       Dark,       {{{0,Minor}, {3,Minor}, {4,Dim}, {6,Major}}}},
      {"Hollow City",    Dark,       {{{5,Minor}, {4,Minor}, {0,Min9}, {6,Dom7}}}},
      {"Nightfall",      Dark,       {{{0,Min7}, {1,Major}, {5,Min7}, {4,Dom7}}}}
    }};
    return library;
}
inline const char* moodName(int mood) noexcept {
    constexpr const char* names[]={"REFLECTIVE","UPLIFTING","URBAN","DARK"};
    return names[std::clamp(mood,0,3)];
}
inline const char* pitchClass(int pc) noexcept {
    constexpr const char* names[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    return names[((pc%12)+12)%12];
}
inline const char* qualityName(Quality q) noexcept {
    switch(q){
      case Major:return "";case Minor:return "m";case Maj7:return "maj7";
      case Min7:return "m7";case Dom7:return "7";case Sus2:return "sus2";
      case Sus4:return "sus4";case Dim:return "dim";case Maj9:return "maj9";
      case Min9:return "m9";
    }
    return "";
}
inline int degreeInterval(int degree, bool minor) noexcept {
    constexpr int major[] = {0,2,4,5,7,9,11};
    constexpr int minorScale[] = {0,2,3,5,7,8,10};
    return minor ? minorScale[std::clamp(degree,0,6)] :
                   major[std::clamp(degree,0,6)];
}
inline std::string chordLabel(int tonic, bool minor, const Chord& chord) {
    const int root=tonic+degreeInterval(chord.degree,minor);
    return std::string(pitchClass(root))+qualityName(chord.quality);
}
struct Settings {
    int preset=0;
    int tonic=0;         // 0=C ... 11=B
    bool minor=false;
    int octave=0;        // -1 to +1
    int inversion=0;     // 0,1,2
    int feel=0;          // flow, stabs, arpeggio, 3-3-2
    int speed=1;         // 0 half, 1 normal, 2 double
    float humanize=0.15f;
    float strumMs=7.0f;
    float velocity=0.78f;
    int take=0;
    float bpm=120.0f;   // required for converting milliseconds to beats
};
struct Note {
    int pitch=60, velocity=90;
    float startBeat=0.0f, durationBeat=1.0f;
};
struct Sequence {
    std::array<Note,256> notes {};
    int count=0;
    float lengthBeats=16.0f;
    void add(Note note) noexcept {
        if(count < static_cast<int>(notes.size()))
            notes[static_cast<size_t>(count++)]=note;
    }
};
inline float randomSigned(uint32_t value) noexcept {
    value ^= value >> 16; value *= 0x7feb352du;
    value ^= value >> 15; value *= 0x846ca68bu;
    value ^= value >> 16;
    return (float(value & 65535u)/32767.5f)-1.0f;
}
inline std::array<int,5> intervals(Quality q, int& count) noexcept {
    count=3;
    switch(q) {
      case Major:return {{0,4,7,0,0}};
      case Minor:return {{0,3,7,0,0}};
      case Maj7:count=4;return {{0,4,7,11,0}};
      case Min7:count=4;return {{0,3,7,10,0}};
      case Dom7:count=4;return {{0,4,7,10,0}};
      case Sus2:return {{0,2,7,0,0}};
      case Sus4:return {{0,5,7,0,0}};
      case Dim:return {{0,3,6,0,0}};
      case Maj9:count=5;return {{0,4,7,11,14}};
      case Min9:count=5;return {{0,3,7,10,14}};
    }
    return {{0,4,7,0,0}};
}
inline Sequence makeSequence(Settings s) noexcept {
    s.preset=std::clamp(s.preset,0,15);
    s.tonic=std::clamp(s.tonic,0,11);
    s.inversion=std::clamp(s.inversion,0,2);
    s.octave=std::clamp(s.octave,-1,1);
    s.feel=std::clamp(s.feel,0,3);
    s.speed=std::clamp(s.speed,0,2);
    s.bpm=std::clamp(s.bpm,40.0f,260.0f);
    s.humanize=std::clamp(s.humanize,0.0f,1.0f);
    s.strumMs=std::clamp(s.strumMs,0.0f,90.0f);
    s.velocity=std::clamp(s.velocity,0.05f,1.0f);
    const float scale=(s.speed==0?2.0f:(s.speed==2?0.5f:1.0f));
    Sequence seq;
    seq.lengthBeats=16.0f*scale;
    const auto& preset=presets()[static_cast<size_t>(s.preset)];
    for (int bar=0;bar<4;++bar) {
        const auto chord=preset.chords[static_cast<size_t>(bar)];
        const int base=48+s.tonic+degreeInterval(chord.degree,s.minor)+12*s.octave;
        int count=0;
        const auto tones=intervals(chord.quality,count);
        std::array<int,5> pitches {};
        for(int i=0;i<count;++i){
            pitches[static_cast<size_t>(i)]=base+tones[static_cast<size_t>(i)];
            if(i<s.inversion)pitches[static_cast<size_t>(i)]+=12;
        }
        std::sort(pitches.begin(),pitches.begin()+count);
        const float beat=float(bar)*4.0f;
        const int hits=s.feel==Flow?1:(s.feel==Arpeggio?8:(s.feel==Stabs?4:3));
        for(int hit=0;hit<hits;++hit) {
            float local=0.0f, span=0.0f;
            if(s.feel==Flow) { local=0.0f;span=3.85f; }
            else if(s.feel==Stabs) {local=float(hit);span=0.67f;}
            else if(s.feel==Arpeggio) {local=0.5f*float(hit);span=0.44f;}
            else {local=hit==0?0.0f:(hit==1?1.5f:3.0f);span=hit==2?0.82f:1.15f;}
            const int begin=s.feel==Arpeggio?hit%count:0;
            const int end=s.feel==Arpeggio?begin+1:count;
            for(int i=begin;i<end;++i){
                const uint32_t seed=uint32_t(s.take+103u)*1013904223u+
                    uint32_t(bar*131+hit*17+i)*747796405u;
                const float jit=randomSigned(seed)*s.humanize*0.045f;
                const float strum=float(i)*(s.strumMs*s.bpm/60000.0f);
                const float offset=(beat+local+strum+jit)*scale;
                const float start=std::clamp(offset,0.0f,seq.lengthBeats-0.02f);
                const float dur=std::max(0.01f,std::min(span*scale,seq.lengthBeats-start-0.005f));
                const float velVar=randomSigned(seed+7919u)*s.humanize*18.0f;
                const int velocity=std::clamp(int(std::round(s.velocity*120.0f+velVar)),1,127);
                seq.add({std::clamp(pitches[static_cast<size_t>(i)],24,108),
                         velocity,start,dur});
            }
        }
    }
    std::sort(seq.notes.begin(),seq.notes.begin()+seq.count,
              [](const Note&a,const Note&b){return a.startBeat < b.startBeat;});
    return seq;
}
} // namespace diochord
