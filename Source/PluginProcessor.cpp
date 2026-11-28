#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr double twoPi=6.2831853071795864769;
std::unique_ptr<juce::AudioParameterChoice> choice(const char* id,
        const char* label,juce::StringArray labels,int initial) {
    return std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID(id,1),label,std::move(labels),initial);
}
std::unique_ptr<juce::AudioParameterInt> integer(const char* id,
        const char* label,int lo,int hi,int initial) {
    return std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID(id,1),label,lo,hi,initial);
}
std::unique_ptr<juce::AudioParameterFloat> percent(const char* id,
        const char* label,float initial,float lo=0,float hi=100) {
    return std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(id,1),label,juce::NormalisableRange<float>(lo,hi,1.0f),initial);
}
}

DioChordProcessor::DioChordProcessor()
    : juce::AudioProcessor(BusesProperties()
          .withOutput("Output",juce::AudioChannelSet::stereo(),true)),
      parameters(*this,nullptr,"DIOCHORD_X",makeLayout()) {}

juce::AudioProcessorValueTreeState::ParameterLayout DioChordProcessor::makeLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout result;
    juce::StringArray names;
    for(const auto& p:diochord::presets())names.add(p.name);
    result.add(choice("progression","Progression",names,8));
    result.add(choice("mood","Mood",{"Reflective","Uplifting","Urban","Dark"},2));
    result.add(choice("key","Key",{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"},0));
    result.add(choice("scale","Scale",{"Major","Minor"},1));
    result.add(integer("octave","Octave",-1,1,0));
    result.add(integer("inversion","Inversion",0,2,0));
    result.add(choice("feel","Feel",{"Flow","Stabs","Arpeggio","3-3-2"},0));
    result.add(choice("speed","Speed",{"Half","Normal","Double"},1));
    result.add(choice("sound","Audition Sound",{"Studio Keys","Electric Keys","Warm Pad","MIDI Only"},0));
    result.add(percent("humanize","Humanize (%)",15));
    result.add(percent("velocity","Velocity (%)",78));
    result.add(percent("strum","Strum (ms)",7,0,90));
    result.add(integer("take","Humanize Take",0,99,0));
    result.add(percent("level","Audition Level (%)",72));
    return result;
}

float DioChordProcessor::raw(const char* id) const noexcept {
    if(auto* value=parameters.getRawParameterValue(id))
        return value->load(std::memory_order_relaxed);
    return 0.0f;
}
diochord::Settings DioChordProcessor::currentSettings(float overrideBpm) const noexcept {
    diochord::Settings s;
    s.preset=static_cast<int>(raw("progression"));
    s.tonic=static_cast<int>(raw("key"));
    s.minor=raw("scale")>0.5f;
    s.octave=static_cast<int>(raw("octave"));
    s.inversion=static_cast<int>(raw("inversion"));
    s.feel=static_cast<int>(raw("feel"));
    s.speed=static_cast<int>(raw("speed"));
    s.humanize=raw("humanize")*0.01f;
    s.strumMs=raw("strum");
    s.velocity=raw("velocity")*0.01f;
    s.take=static_cast<int>(raw("take"));
    s.bpm=overrideBpm>0.0f?overrideBpm:getHostBpm();
    return s;
}

void DioChordProcessor::prepareToPlay(double rate,int) {
    clockRate=std::max(8000.0,rate);
    freeBeat=0.0f;
    previousPlaying=false;
    stopAllNotes();
}
void DioChordProcessor::releaseResources() {
    stopPreview();
    stopAllNotes();
}
bool DioChordProcessor::isBusesLayoutSupported(const BusesLayout& layout) const {
    const auto output=layout.getMainOutputChannelSet();
    return output==juce::AudioChannelSet::mono()
        || output==juce::AudioChannelSet::stereo();
}
void DioChordProcessor::noteOn(int pitch,int velocity) noexcept {
    Voice* target=nullptr;
    for(auto& voice:voices) {
        if(voice.pitch<0||voice.envelope<0.0001f) {target=&voice;break;}
    }
    if(!target)target=&*std::min_element(voices.begin(),voices.end(),
        [](const Voice& a,const Voice& b){return a.serial<b.serial;});
    target->pitch=pitch;
    target->phase=0.0;
    target->frequency=440.0*std::pow(2.0,double(pitch-69)/12.0);
    target->envelope=0.0f;
    target->velocity=float(velocity)/127.0f;
    target->releasing=false;
    target->serial=serial++;
}
void DioChordProcessor::noteOff(int pitch) noexcept {
    for(auto& v:voices)if(v.pitch==pitch)v.releasing=true;
}
void DioChordProcessor::stopAllNotes() noexcept {
    for(auto& v:voices){v.pitch=-1;v.envelope=0.0f;v.releasing=true;}
}
float DioChordProcessor::synthSample(int timbre) noexcept {
    if(timbre>=3)return 0.0f; // MIDI Only
    float sum=0.0f;
    for(auto& v:voices) {
        if(v.pitch<0)continue;
        const float attack=timbre==2?0.00022f:0.01f;
        if(v.releasing)v.envelope*=timbre==2?0.99990f:0.99957f;
        else if(timbre==2)v.envelope+=(1.0f-v.envelope)*attack;
        else v.envelope+=(1.0f-v.envelope)*attack;
        if(timbre==0)v.envelope*=0.99990f;
        if(timbre==1)v.envelope*=0.99996f;
        if(v.envelope<0.00003f) {
            v.pitch=-1;continue;
        }
        const float ph=static_cast<float>(v.phase);
        float sound=std::sin(ph);
        if(timbre==0)sound=0.83f*sound+0.15f*std::sin(ph*2.0f)+0.06f*std::sin(ph*3.0f);
        else if(timbre==1)sound=0.78f*sound+0.19f*std::sin(ph*3.0f)+0.05f*std::sin(ph*5.0f);
        else sound=0.85f*sound+0.12f*std::sin(ph*2.003f);
        sum+=sound*v.envelope*v.velocity*0.12f;
        v.phase+=twoPi*v.frequency/clockRate;
        if(v.phase>=twoPi)v.phase=std::fmod(v.phase,twoPi);
    }
    return std::tanh(sum*1.0f)*0.82f;
}

void DioChordProcessor::queueTriggers(const diochord::Sequence& seq,
        double startBeat,double endBeat,double samplesPerBeat,int blockSize,
        std::array<Trigger,512>& triggers,int& count) const noexcept {
    const double cycle=std::max(1.0,double(seq.lengthBeats));
    auto add=[&](const diochord::Note& n,bool on,double relative) {
        if(count>=static_cast<int>(triggers.size()))return;
        const double repeat=std::floor((startBeat-relative)/cycle);
        double absolute=repeat*cycle+relative;
        if(absolute<startBeat-1e-7)absolute+=cycle;
        if(absolute>=endBeat-1e-7)return;
        const int index=std::clamp(
            static_cast<int>(std::floor((absolute-startBeat)*samplesPerBeat+0.5)),
            0,blockSize-1);
        triggers[static_cast<size_t>(count++)]={index,n.pitch,n.velocity,on};
    };
    for(int i=0;i<seq.count;++i) {
        const auto& n=seq.notes[static_cast<size_t>(i)];
        add(n,true,n.startBeat);
        add(n,false,double(n.startBeat)+double(n.durationBeat));
    }
    std::sort(triggers.begin(),triggers.begin()+count,
        [](const Trigger&a,const Trigger&b){
            if(a.sample!=b.sample)return a.sample<b.sample;
            return int(a.on)<int(b.on); // note off before note on
        });
}

void DioChordProcessor::processBlock(juce::AudioBuffer<float>& audio,juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    const int n=audio.getNumSamples();
    for(int c=0;c<audio.getNumChannels();++c)audio.clear(c,0,n);
    midi.clear();
    if(n<=0||audio.getNumChannels()<=0)return;
    float bpm=getHostBpm();
    bool hostPlaying=false;
    double hostBeat=0.0;
    if(auto* playhead=getPlayHead()){
        if(auto position=playhead->getPosition()) {
            hostPlaying=position->getIsPlaying();
            if(auto hostTempo=position->getBpm()) {
                if(std::isfinite(*hostTempo)&&*hostTempo>0.0)
                    bpm=std::clamp(float(*hostTempo),40.0f,260.0f);
            }
            if(auto ppq=position->getPpqPosition())hostBeat=*ppq;
        }
    }
    currentBpm.store(bpm,std::memory_order_relaxed);
    if(restartPreview.exchange(false)) {
        freeBeat=0.0f;
        stopAllNotes();
    }
    const bool preview=previewActive.load(std::memory_order_relaxed);
    const bool playing=hostPlaying||preview;
    if(!playing) {
        if(previousPlaying) {
            midi.addEvent(juce::MidiMessage::allNotesOff(1),0);
            stopAllNotes();
        }
        previousPlaying=false;
        currentBeat.store(0.0f,std::memory_order_relaxed);
        return;
    }
    const int p=static_cast<int>(raw("progression"));
    const int k=static_cast<int>(raw("key"));
    const int m=static_cast<int>(raw("scale"));
    const int f=static_cast<int>(raw("feel"));
    if(p!=lastPreset||k!=lastKey||m!=lastMode||f!=lastFeel||!previousPlaying) {
        stopAllNotes();
        midi.addEvent(juce::MidiMessage::allNotesOff(1),0);
        lastPreset=p;lastKey=k;lastMode=m;lastFeel=f;
    }
    previousPlaying=true;
    const auto seq=diochord::makeSequence(currentSettings(bpm));
    const double beatDelta=double(bpm)/(60.0*clockRate);
    const double startBeat=hostPlaying?hostBeat:double(freeBeat);
    const double endBeat=startBeat+beatDelta*double(n);
    std::array<Trigger,512> triggers{};
    int eventCount=0;
    queueTriggers(seq,startBeat,endBeat,1.0/beatDelta,n,triggers,eventCount);
    float* outL=audio.getWritePointer(0);
    float* outR=audio.getWritePointer(audio.getNumChannels()>1?1:0);
    const float level=std::clamp(raw("level")*0.01f,0.0f,1.0f);
    const int sound=static_cast<int>(raw("sound"));
    int at=0;
    for(int i=0;i<n;++i) {
        while(at<eventCount&&triggers[static_cast<size_t>(at)].sample<=i) {
            const auto& e=triggers[static_cast<size_t>(at++)];
            if(e.on) {
                noteOn(e.pitch,e.velocity);
                midi.addEvent(juce::MidiMessage::noteOn(
                    1,e.pitch,static_cast<juce::uint8>(e.velocity)),i);
            } else {
                noteOff(e.pitch);
                midi.addEvent(juce::MidiMessage::noteOff(1,e.pitch),i);
            }
        }
        const float sample=level*synthSample(sound);
        outL[i]=sample;
        if(audio.getNumChannels()>1)outR[i]=sample;
    }
    if(!hostPlaying)freeBeat=static_cast<float>(endBeat);
    currentBeat.store(static_cast<float>(std::fmod(endBeat,seq.lengthBeats)),
                      std::memory_order_relaxed);
}

void DioChordProcessor::togglePreview() {
    const bool next=!previewActive.load(std::memory_order_relaxed);
    previewActive.store(next,std::memory_order_relaxed);
    if(next)restartPreview.store(true,std::memory_order_release);
}
void DioChordProcessor::stopPreview() {
    previewActive.store(false,std::memory_order_relaxed);
}
bool DioChordProcessor::exportMidiFile(const juce::File& file) {
    const auto seq=diochord::makeSequence(currentSettings());
    juce::MidiMessageSequence track;
    // All chord note events are exported to a standard MIDI track.
    constexpr double ticks=960.0;
    // File is PPQ MIDI: the DAW project tempo determines playback speed.
    for(int i=0;i<seq.count;++i) {
        const auto& note=seq.notes[static_cast<size_t>(i)];
        auto on=juce::MidiMessage::noteOn(1,note.pitch,
                                        static_cast<juce::uint8>(note.velocity));
        on.setTimeStamp(double(note.startBeat)*ticks);
        track.addEvent(on);
        auto off=juce::MidiMessage::noteOff(1,note.pitch);
        off.setTimeStamp((double(note.startBeat)+double(note.durationBeat))*ticks);
        track.addEvent(off);
    }
    track.sort();
    track.updateMatchedPairs();
    juce::MidiFile midi;
    midi.setTicksPerQuarterNote(960);
    midi.addTrack(track);
    if(file.existsAsFile())file.deleteFile();
    auto stream=file.createOutputStream();
    return stream&&stream->openedOk()&&midi.writeTo(*stream);
}
juce::File DioChordProcessor::exportMidiToTemporaryFile() {
    const auto folder=juce::File::getSpecialLocation(
        juce::File::userApplicationDataDirectory)
        .getChildFile("DioCHORD X").getChildFile("Exports");
    if(folder.createDirectory().failed())return {};
    const auto preset=diochord::presets()[static_cast<size_t>(
        std::clamp(int(raw("progression")),0,15))];
    const auto path=folder.getNonexistentChildFile(
        "DioCHORD_X_"+juce::File::createLegalFileName(preset.name)+"_"+
            juce::String(++exportIndex),".mid",false);
    return exportMidiFile(path)?path:juce::File();
}
void DioChordProcessor::getStateInformation(juce::MemoryBlock& out) {
    if(auto xml=parameters.copyState().createXml())copyXmlToBinary(*xml,out);
}
void DioChordProcessor::setStateInformation(const void* data,int size) {
    if(auto xml=getXmlFromBinary(data,size)) {
        auto state=juce::ValueTree::fromXml(*xml);
        if(state.isValid()&&state.hasType(parameters.state.getType()))
            parameters.replaceState(state);
    }
}
juce::AudioProcessorEditor* DioChordProcessor::createEditor() {
    return new DioChordEditor(*this);
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new DioChordProcessor();
}
