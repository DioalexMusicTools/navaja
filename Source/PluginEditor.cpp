#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace {
const juce::Colour bg(0xff080713);
const juce::Colour panel(0xff131127);
const juce::Colour surface(0xff1b1835);
const juce::Colour line(0xff403957);
const juce::Colour pale(0xfff3f2fc);
const juce::Colour muted(0xffaaa6c5);
const juce::Colour violet(0xff9662fc);
const juce::Colour purple(0xffbd4efb);
const juce::Colour blue(0xff547bfa);
const juce::Colour cyan(0xff24c7fa);
const juce::Colour green(0xff70e2c2);
void font(juce::Graphics&g,float size,bool bold=false){
    g.setFont(juce::Font(juce::FontOptions(size,
        bold?juce::Font::bold:juce::Font::plain)));
}
void section(juce::Graphics&g,juce::Rectangle<float> bounds,juce::Colour accent,
             const juce::String& title) {
    g.setColour(panel);
    g.fillRoundedRectangle(bounds,12.0f);
    g.setColour(accent.withAlpha(0.75f));
    g.drawRoundedRectangle(bounds.reduced(0.8f),12.0f,1.6f);
    g.setColour(pale);font(g,12.0f,true);
    g.drawText(title,bounds.toNearestInt().reduced(12).removeFromTop(21),
               juce::Justification::centredLeft);
}
void caption(juce::Graphics&g,const char*name,int x,int y,int w,juce::Colour color=muted){
    g.setColour(color);font(g,11.0f,true);
    g.drawText(name,x,y,w,19,juce::Justification::centred);
}
}

DioChordLookAndFeel::DioChordLookAndFeel(){
    setColour(juce::Slider::textBoxTextColourId,pale);
    setColour(juce::Slider::textBoxBackgroundColourId,bg);
    setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId,surface);
    setColour(juce::ComboBox::textColourId,pale);
    setColour(juce::ComboBox::outlineColourId,line);
    setColour(juce::PopupMenu::backgroundColourId,surface);
    setColour(juce::PopupMenu::textColourId,pale);
    setColour(juce::PopupMenu::highlightedBackgroundColourId,violet);
    setColour(juce::PopupMenu::highlightedTextColourId,bg);
    setColour(juce::TextButton::buttonColourId,surface);
    setColour(juce::TextButton::textColourOffId,pale);
}

void DioChordLookAndFeel::drawRotarySlider(juce::Graphics&g,int x,int y,int w,int h,
                         float pos,float start,float end,juce::Slider& slider){
    const float cx=float(x)+0.5f*float(w);
    const float cy=float(y)+0.41f*float(h);
    const float radius=float(std::min(w,h))*0.32f;
    const auto accent=slider.findColour(juce::Slider::rotarySliderFillColourId,true);
    juce::Path backgroundArc,fillArc;
    backgroundArc.addCentredArc(cx,cy,radius+6,radius+6,0,start,end,true);
    fillArc.addCentredArc(cx,cy,radius+6,radius+6,0,start,start+pos*(end-start),true);
    g.setColour(juce::Colour(0xff383252));
    g.strokePath(backgroundArc,juce::PathStrokeType(4.5f));
    g.setColour(accent);
    g.strokePath(fillArc,juce::PathStrokeType(5.0f));
    g.setColour(juce::Colour(0xff26223f));
    g.fillEllipse(cx-radius,cy-radius,radius*2,radius*2);
    const float angle=start+pos*(end-start);
    g.setColour(pale);
    g.drawLine(cx+std::sin(angle)*radius*0.22f,cy-std::cos(angle)*radius*0.22f,
               cx+std::sin(angle)*radius*0.76f,cy-std::cos(angle)*radius*0.76f,
               3.0f);
}

void DioChordEditor::DragPad::paint(juce::Graphics&g){
    const auto bounds=getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(cyan);
    g.fillRoundedRectangle(bounds,10.0f);
    g.setColour(bg);
    font(g,17,true);
    g.drawText("DRAG MIDI TO TRACK",getLocalBounds().reduced(8),
               juce::Justification::centred);
}
void DioChordEditor::DragPad::mouseDrag(const juce::MouseEvent&e){
    if(!armed||e.getDistanceFromDragStart()<8||!processor)return;
    armed=false;
    const auto file=processor->exportMidiToTemporaryFile();
    if(file.existsAsFile())
        juce::DragAndDropContainer::performExternalDragDropOfFiles(
            juce::StringArray{file.getFullPathName()},false,this);
}

DioChordEditor::DioChordEditor(DioChordProcessor&p)
    : AudioProcessorEditor(&p),processor(p){
    setLookAndFeel(&look);
    const char* moods[]={"REFLECTIVE","UPLIFTING","URBAN","DARK"};
    for(int i=0;i<4;++i){
        auto& b=moodButtons[static_cast<size_t>(i)];
        b.setButtonText(moods[i]);
        b.onClick=[this,i]{
            visibleMood=i;
            setParameter("mood",float(i));
            setParameter("progression",float(i*4));
            updateCards();
            repaint();
        };
        addAndMakeVisible(b);
    }
    for(int i=0;i<4;++i){
        auto& b=progressionButtons[static_cast<size_t>(i)];
        b.onClick=[this,i]{
            setParameter("progression",float(visibleMood*4+i));
            updateCards();
            repaint();
        };
        addAndMakeVisible(b);
    }
    auto& ps=processor.parameters;
    key.addItemList({"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"},1);
    scale.addItemList({"MAJOR","MINOR"},1);
    octave.addItemList({"-1","0","+1"},1);
    inversion.addItemList({"ROOT","FIRST","SECOND"},1);
    speed.addItemList({"1/2x","1x","2x"},1);
    feel.addItemList({"FLOW","STABS","ARPEGGIO","3-3-2"},1);
    sound.addItemList({"STUDIO KEYS","ELECTRIC KEYS","WARM PAD","MIDI ONLY"},1);
    comboLinks[0]=std::make_unique<ComboLink>(ps,"key",key);
    comboLinks[1]=std::make_unique<ComboLink>(ps,"scale",scale);
    comboLinks[2]=std::make_unique<ComboLink>(ps,"octave",octave);
    comboLinks[3]=std::make_unique<ComboLink>(ps,"inversion",inversion);
    comboLinks[4]=std::make_unique<ComboLink>(ps,"speed",speed);
    comboLinks[5]=std::make_unique<ComboLink>(ps,"feel",feel);
    comboLinks[6]=std::make_unique<ComboLink>(ps,"sound",sound);
    for(auto* combo:{&key,&scale,&octave,&inversion,&speed,&feel,&sound})
        addAndMakeVisible(*combo);
    setupKnob(humanize,"humanize",knobLinks[0]);
    setupKnob(strum,"strum",knobLinks[1]);
    setupKnob(velocity,"velocity",knobLinks[2]);
    setupKnob(level,"level",knobLinks[3]);
    humanize.setColour(juce::Slider::rotarySliderFillColourId,violet);
    strum.setColour(juce::Slider::rotarySliderFillColourId,purple);
    velocity.setColour(juce::Slider::rotarySliderFillColourId,blue);
    level.setColour(juce::Slider::rotarySliderFillColourId,cyan);
    play.onClick=[this]{processor.togglePreview();repaint();};
    newTake.onClick=[this]{
        if(auto* parameter=processor.parameters.getParameter("take")){
            const int next=(int(processor.parameters.getRawParameterValue("take")->load())+1)%100;
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost(parameter->convertTo0to1(float(next)));
            parameter->endChangeGesture();
        }
    };
    save.onClick=[this]{saveMidiDialog();};
    for(auto*b:{&play,&newTake,&save})addAndMakeVisible(*b);
    play.setColour(juce::TextButton::buttonColourId,violet);
    play.setColour(juce::TextButton::textColourOffId,bg);
    save.setColour(juce::TextButton::buttonColourId,blue);
    save.setColour(juce::TextButton::textColourOffId,pale);
    dragPad.processor=&processor;
    addAndMakeVisible(dragPad);
    visibleMood=std::clamp(int(ps.getRawParameterValue("mood")->load()),0,3);
    updateCards();
    setSize(1280,824);
    startTimerHz(24);
}
DioChordEditor::~DioChordEditor(){
    stopTimer();
    chooser.reset();
    setLookAndFeel(nullptr);
}
void DioChordEditor::setParameter(const char*id,float actual){
    if(auto* p=processor.parameters.getParameter(id)){
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1(actual));
        p->endChangeGesture();
    }
}
void DioChordEditor::updateCards(){
    const int chosen=std::clamp(int(processor.parameters
                         .getRawParameterValue("progression")->load()),0,15);
    for(int i=0;i<4;++i){
        const auto& preset=diochord::presets()[static_cast<size_t>(visibleMood*4+i)];
        auto& button=progressionButtons[static_cast<size_t>(i)];
        button.setButtonText(preset.name);
        const bool active=visibleMood*4+i==chosen;
        button.setColour(juce::TextButton::buttonColourId,active?
                         juce::Colour(0xff403272):surface);
        button.setColour(juce::TextButton::textColourOffId,active?cyan:pale);
    }
    for(int i=0;i<4;++i)
        moodButtons[static_cast<size_t>(i)].setColour(
            juce::TextButton::buttonColourId,i==visibleMood?violet:surface);
    lastPreset=chosen;
}
void DioChordEditor::setupKnob(juce::Slider&s,const char*id,
                               std::unique_ptr<KnobLink>& link){
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setRotaryParameters(juce::MathConstants<float>::pi*1.25f,
                          juce::MathConstants<float>::pi*2.75f,false);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,84,20);
    s.setMouseDragSensitivity(175);
    s.setPopupDisplayEnabled(true,false,this);
    s.textFromValueFunction=[id](double v){
        return juce::String(juce::roundToInt(v))+
               (juce::String(id)=="strum"?" ms":"%");
    };
    s.valueFromTextFunction=[](const juce::String&text){
        return text.retainCharacters("-.0123456789").getDoubleValue();
    };
    if(auto* p=processor.parameters.getParameter(id))
        s.setDoubleClickReturnValue(true,p->convertFrom0to1(p->getDefaultValue()));
    addAndMakeVisible(s);
    link=std::make_unique<KnobLink>(processor.parameters,id,s);
}
void DioChordEditor::timerCallback(){
    const int selected=int(processor.parameters
                           .getRawParameterValue("progression")->load());
    const int mood=int(processor.parameters.getRawParameterValue("mood")->load());
    if(mood!=visibleMood||selected!=lastPreset){
        visibleMood=std::clamp(mood,0,3);
        updateCards();
    }
    cursor=processor.getPlayPosition();
    play.setButtonText(processor.isPreviewing()?"STOP":"PLAY");
    repaint(20,327,1240,204);
    repaint(1078,18,177,65);
}
void DioChordEditor::paint(juce::Graphics&g){
    g.fillAll(bg);
    g.setColour(juce::Colour(0xff100d23));
    g.fillRoundedRectangle(juce::Rectangle<float>(10,9,1260,807),14.0f);
    font(g,43.0f,true);
    g.setColour(pale);
    g.drawText("Dio",34,23,93,59,juce::Justification::centredLeft);
    g.setColour(violet);
    g.drawText("CHORD",120,23,213,59,juce::Justification::centredLeft);
    g.setColour(cyan);
    g.drawText("X",320,23,55,59,juce::Justification::centredLeft);
    g.setColour(muted);font(g,12.0f,true);
    g.drawText("MOOD PROGRESSION STUDIO  /  DIOALEX MUSIC TOOLS",
               404,41,566,25,juce::Justification::centredLeft);
    g.setColour(cyan);
    g.fillRoundedRectangle(juce::Rectangle<float>(1072,29,174,53),8.0f);
    g.setColour(bg);font(g,17.0f,true);
    g.drawText(juce::String(juce::roundToInt(processor.getHostBpm()))+" BPM",
               1080,34,159,39,juce::Justification::centred);
    g.setColour(panel);
    g.fillRoundedRectangle(juce::Rectangle<float>(23,115,1234,65),8.0f);
    g.setColour(muted);
    font(g,12,true);
    g.drawText("MOOD",34,121,72,17,juce::Justification::centredLeft);
    g.drawText("KEY",802,121,90,17,juce::Justification::centredLeft);
    g.drawText("SCALE",966,121,90,17,juce::Justification::centredLeft);

    g.setColour(pale);font(g,15.0f,true);
    g.drawText("EXPLORE  /  ORIGINAL PROGRESSIONS",
               31,190,677,22,juce::Justification::centredLeft);
    g.setColour(muted);font(g,11);
    g.drawText("Select a color palette for your next chord idea",
               746,191,478,21,juce::Justification::centredRight);
    for(int i=0;i<4;++i){
        const int x=24+i*308;
        const bool active=visibleMood*4+i==lastPreset;
        const auto& preset=diochord::presets()[static_cast<size_t>(visibleMood*4+i)];
        g.setColour(active?violet:line);
        g.drawRoundedRectangle(juce::Rectangle<float>(float(x),219,297,101),9.0f,1.8f);
        g.setColour(active?cyan:muted);
        font(g,10,true);
        g.drawText("0"+juce::String(i+1),x+17,284,35,18,
                   juce::Justification::centredLeft);
        g.setColour(pale);font(g,11);
        const auto keyValue=std::clamp(int(processor.parameters
                        .getRawParameterValue("key")->load()),0,11);
        const bool isMinor=processor.parameters.getRawParameterValue("scale")->load()>0.5f;
        juce::String desc;
        for(int c=0;c<4;++c){
            if(c)desc+="  •  ";
            desc+=diochord::chordLabel(keyValue,isMinor,preset.chords[static_cast<size_t>(c)]);
        }
        g.drawFittedText(desc,x+52,285,236,17,juce::Justification::centredLeft,1);
    }

    section(g,{23,340,1234,180},blue,"PROGRESSION EDITOR    /    4 BARS");
    const int selected=std::clamp(int(processor.parameters
                                  .getRawParameterValue("progression")->load()),0,15);
    const auto& preset=diochord::presets()[static_cast<size_t>(selected)];
    const int tonic=std::clamp(int(processor.parameters.getRawParameterValue("key")->load()),0,11);
    const bool minor=processor.parameters.getRawParameterValue("scale")->load()>0.5f;
    const float speedValue=processor.parameters.getRawParameterValue("speed")->load();
    const float speed= speedValue<0.5f?2.0f:(speedValue>1.5f?0.5f:1.0f);
    const float phase=std::fmod(std::max(0.0f,cursor),16.0f*speed)/(16.0f*speed);
    for(int i=0;i<4;++i){
        const int x=43+i*304;
        g.setColour(juce::Colour(0xff221e42));
        g.fillRoundedRectangle(juce::Rectangle<float>(float(x),378,290,118),6.0f);
        g.setColour(i==int(phase*4.0f)?cyan:violet);
        g.fillRoundedRectangle(juce::Rectangle<float>(float(x),378,290,3),1.0f);
        g.setColour(pale);font(g,23,true);
        const juce::String name=diochord::chordLabel(tonic,minor,
                                    preset.chords[static_cast<size_t>(i)]);
        g.drawText(name,x+12,389,245,34,juce::Justification::centredLeft);
        g.setColour(muted);font(g,11,true);
        g.drawText("BAR "+juce::String(i+1),x+208,392,67,25,
                   juce::Justification::centredRight);
        const auto seq=diochord::makeSequence(processor.currentSettings());
        int drawn=0;
        for(int q=0;q<seq.count&&drawn<5;++q){
            const auto& note=seq.notes[static_cast<size_t>(q)];
            if(note.startBeat>=float(i+1)*4.0f*speed ||
               note.startBeat<float(i)*4.0f*speed)continue;
            const float relative=(note.startBeat-float(i)*4.0f*speed)/(4.0f*speed);
            const float nx=float(x)+12.0f+relative*207.0f;
            const float ny=432.0f+float(drawn)*8.0f;
            g.setColour(drawn%2==0?violet:blue);
            g.fillRoundedRectangle(juce::Rectangle<float>(
                nx,ny,std::max(5.0f,65.0f*note.durationBeat/speed/4.0f),4.0f),2.0f);
            ++drawn;
        }
    }
    const float cursorX=43.0f+phase*1216.0f;
    g.setColour(cyan.withAlpha(0.85f));
    g.drawVerticalLine(juce::roundToInt(cursorX),378,498);

    section(g,{23,531,253,212},violet,"HARMONY");
    section(g,{287,531,589,212},purple,"PERFORMANCE / HUMANIZE");
    section(g,{887,531,370,212},cyan,"PLAYBACK / MIDI");
    caption(g,"OCTAVE",37,560,105);
    caption(g,"INVERSION",150,560,112);
    caption(g,"SPEED",37,643,105);
    caption(g,"HUMANIZE",311,563,115);
    caption(g,"STRUM",457,563,115);
    caption(g,"VELOCITY",606,563,115);
    caption(g,"LEVEL",752,563,115);
    caption(g,"FEEL",902,563,161);
    caption(g,"SOUND",1080,563,162);
    g.setColour(muted);font(g,10.5f);
    g.drawText("SYNC TO DAW  •  NOTES + VELOCITY + TIMING",
               915,682,321,18,juce::Justification::centred);
    g.setColour(surface);
    g.fillRoundedRectangle(juce::Rectangle<float>(23,753,1234,53),9.0f);
    g.setColour(muted);font(g,11);
    g.drawText("Drag MIDI to another instrument • SAVE MIDI is available if your DAW blocks native dragging",
               37,784,812,18,juce::Justification::centredLeft);
}
void DioChordEditor::resized(){
    for(int i=0;i<4;++i)
        moodButtons[static_cast<size_t>(i)].setBounds(36+i*143,141,135,30);
    key.setBounds(800,140,147,32);
    scale.setBounds(963,140,138,32);
    for(int i=0;i<4;++i)
        progressionButtons[static_cast<size_t>(i)].setBounds(40+i*308,234,263,42);
    octave.setBounds(39,582,103,35);
    inversion.setBounds(151,582,111,35);
    speed.setBounds(39,667,103,35);
    std::array<juce::Slider*,4> knobs{{&humanize,&strum,&velocity,&level}};
    for(int i=0;i<4;++i)
        knobs[static_cast<size_t>(i)]->setBounds(307+i*149,585,131,117);
    feel.setBounds(902,586,161,34);
    sound.setBounds(1080,586,162,34);
    play.setBounds(904,639,154,37);
    newTake.setBounds(1076,639,160,37);
    save.setBounds(875,760,160,37);
    dragPad.setBounds(1049,760,196,37);
}

void DioChordEditor::saveMidiDialog(){
    chooser=std::make_unique<juce::FileChooser>("Save DioCHORD X MIDI",
             juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                    .getChildFile("DioCHORD_X.mid"),"*.mid");
    juce::Component::SafePointer<DioChordEditor> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::saveMode |
                         juce::FileBrowserComponent::canSelectFiles,
           [safe](const juce::FileChooser& browser){
              if(!safe)return;
              auto f=browser.getResult();
              if(f==juce::File())return;
              safe->processor.exportMidiFile(f.withFileExtension(".mid"));
           });
}
