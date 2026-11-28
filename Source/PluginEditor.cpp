#include "PluginEditor.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace {
const juce::Colour bg(0xff080715);
const juce::Colour panel(0xff111025);
const juce::Colour surface(0xff17152e);
const juce::Colour labelWhite(0xfff3f1ff);
const juce::Colour muted(0xffa4a1c5);
const juce::Colour purple(0xff8d62ff);
const juce::Colour violet(0xffb43fff);
const juce::Colour royal(0xff587aff);
const juce::Colour cyan(0xff18c8fa);
const juce::Colour teal(0xff23d3da);
const juce::Colour darkStroke(0xff373257);
void font(juce::Graphics& g, float px, bool heavy = false) {
    g.setFont(juce::Font(juce::FontOptions(px,
        heavy ? juce::Font::bold : juce::Font::plain)));
}
void label(juce::Graphics& g, const char* s, int x, int y, int w, juce::Colour c = labelWhite) {
    g.setColour(c);
    font(g, 12.0f, true);
    g.drawText(s, x, y, w, 19, juce::Justification::centred);
}
void panelBox(juce::Graphics& g, int x, int y, int w, int h, juce::Colour color, const char* title) {
    auto a=juce::Rectangle<float>(float(x),float(y),float(w),float(h));
    g.setColour(panel);
    g.fillRoundedRectangle(a,13.0f);
    g.setColour(color);
    g.drawRoundedRectangle(a.reduced(1.1f),13.0f,2.25f);
    g.setColour(labelWhite);
    font(g,15.5f,true);
    g.drawText(title,x+11,y+11,w-22,22,juce::Justification::centred);
}
juce::String formatted(const juce::String& id,double value) {
    if(id=="decayMs"||id=="predelayMs")
        return juce::String(juce::roundToInt(value))+" ms";
    if(id=="highFreq"||id=="dampHighHz"||id=="bassFreqHz"||
       id=="highCutHz"||id=="lowCutHz") {
        if(value>=1000.0) return juce::String(value*0.001,2)+" kHz";
        return juce::String(juce::roundToInt(value))+" Hz";
    }
    if(id=="modRateHz") return juce::String(value,2)+" Hz";
    if(id=="highShelfDb"||id=="output")
        return (value>0?"+":"")+juce::String(value,1)+" dB";
    if(id=="bassMult") return juce::String(value,2)+" x";
    return juce::String(juce::roundToInt(value))+"%";
}
}

DioVerbLookAndFeel::DioVerbLookAndFeel() {
    setColour(juce::Slider::textBoxTextColourId,labelWhite);
    setColour(juce::Slider::textBoxBackgroundColourId,bg);
    setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId,surface);
    setColour(juce::ComboBox::textColourId,labelWhite);
    setColour(juce::ComboBox::outlineColourId,darkStroke);
    setColour(juce::PopupMenu::backgroundColourId,surface);
    setColour(juce::PopupMenu::textColourId,labelWhite);
    setColour(juce::PopupMenu::highlightedBackgroundColourId,purple);
    setColour(juce::PopupMenu::highlightedTextColourId,bg);
    setColour(juce::TextButton::buttonColourId,surface);
    setColour(juce::TextButton::textColourOffId,labelWhite);
}

void DioVerbLookAndFeel::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,
                                         float pos,float start,float end,juce::Slider& slider) {
    const float cx=float(x)+float(w)*0.5f;
    const float cy=float(y)+float(h)*0.43f;
    const auto accent=slider.findColour(juce::Slider::rotarySliderFillColourId,true);
    if(slider.getComponentID()=="decayOrbit") {
        const float radius=std::min(float(w)*0.42f,float(h)*0.40f);
        for(int ring=0;ring<5;++ring) {
            const float r=radius-ring*15.0f;
            if(r<12) break;
            juce::Path back,active;
            back.addCentredArc(cx,cy,r,r,0.0f,start,end,true);
            active.addCentredArc(cx,cy,r,r,0.0f,start,start+(end-start)*pos,true);
            g.setColour(juce::Colour(0xff393051));
            g.strokePath(back,juce::PathStrokeType(6.0f));
            g.setColour(ring%2==0 ? cyan.withAlpha(0.87f) : accent.withAlpha(0.94f));
            g.strokePath(active,juce::PathStrokeType(6.0f));
        }
        g.setColour(labelWhite);
        font(g,12.0f,true);
        g.drawText("DECAY",int(cx)-56,int(cy)-9,112,19,juce::Justification::centred);
        return;
    }
    const float r=std::min(float(w),float(h))*0.30f;
    juce::Path back,arc;
    back.addCentredArc(cx,cy,r+6,r+6,0,start,end,true);
    arc.addCentredArc(cx,cy,r+6,r+6,0,start,start+(end-start)*pos,true);
    g.setColour(juce::Colour(0xff353251));
    g.strokePath(back,juce::PathStrokeType(5.0f));
    g.setColour(accent);
    g.strokePath(arc,juce::PathStrokeType(5.3f));
    g.setColour(juce::Colour(0xff252141));
    g.fillEllipse(cx-r,cy-r,2*r,2*r);
    const float angle=start+(end-start)*pos;
    g.setColour(labelWhite);
    g.drawLine(cx+std::sin(angle)*r*0.25f,cy-std::cos(angle)*r*0.25f,
               cx+std::sin(angle)*r*0.78f,cy-std::cos(angle)*r*0.78f,3.0f);
}

void DioVerbEditor::DragPad::paint(juce::Graphics& g) {
    const bool ready=fetchFile && fetchFile().existsAsFile();
    const auto r=getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(ready?teal:juce::Colour(0xff403862));
    g.fillRoundedRectangle(r,9.0f);
    g.setColour(ready?bg:muted);
    font(g,13.5f,true);
    g.drawFittedText(ready?"DRAG WAV TO DAW":"RENDER TO ENABLE DRAG",
                     getLocalBounds().reduced(8),juce::Justification::centred,1);
}
void DioVerbEditor::DragPad::mouseDown(const juce::MouseEvent&) { armed=true; }
void DioVerbEditor::DragPad::mouseDrag(const juce::MouseEvent& e) {
    if(!armed || e.getDistanceFromDragStart()<8) return;
    armed=false;
    if(fetchFile) {
        const auto file=fetchFile();
        if(file.existsAsFile())
            juce::DragAndDropContainer::performExternalDragDropOfFiles(
                juce::StringArray{file.getFullPathName()},false,this);
    }
}

DioVerbEditor::DioVerbEditor(DioVerbProcessor& p)
    : AudioProcessorEditor(&p),processor(p) {
    setLookAndFeel(&look);
    auto& ps=processor.parameters;
    mode.addItemList({"VOCAL","INSTRUMENT"},1);
    length.addItemList({"1/4 BAR","1/2 BAR","1 BAR"},1);
    tremolo.addItemList({"OFF","1/8","1/16","1/32"},1);
    autoPan.addItemList({"OFF","1/2","1/4","1/8","1/16"},1);
    choiceLinks[0]=std::make_unique<ChoiceLink>(ps,"mode",mode);
    choiceLinks[1]=std::make_unique<ChoiceLink>(ps,"length",length);
    choiceLinks[2]=std::make_unique<ChoiceLink>(ps,"tremolo",tremolo);
    choiceLinks[3]=std::make_unique<ChoiceLink>(ps,"autoPan",autoPan);
    for(auto* combo : {&mode,&length,&tremolo,&autoPan}) addAndMakeVisible(*combo);

    struct KnobDescriptor {
        juce::Slider* slider;
        const char* id;
        juce::Colour tint;
    };
    const KnobDescriptor descriptors[] = {
        {&mix,"reverbMix",purple}, {&predelay,"predelayMs",purple},
        {&decay,"decayMs",purple}, {&highFreq,"dampHighHz",violet},
        {&highShelf,"highShelfDb",violet}, {&bassFreq,"bassFreqHz",violet},
        {&bassMult,"bassMult",violet}, {&size,"size",purple},
        {&attack,"attack",purple}, {&width,"width",purple},
        {&early,"earlyDiff",royal}, {&late,"lateDiff",royal},
        {&tone,"tone",royal}, {&modRate,"modRateHz",cyan},
        {&modDepth,"modDepth",cyan}, {&tremDepth,"depth",cyan},
        {&panDepth,"panDepth",cyan}, {&highCut,"highCutHz",teal},
        {&lowCut,"lowCutHz",teal}, {&output,"output",teal}
    };
    static_assert(sizeof(descriptors)/sizeof(descriptors[0])==20,"Knob count");
    for(size_t i=0;i<knobLinks.size();++i)
        setupKnob(*descriptors[i].slider,descriptors[i].id,knobLinks[i],
                  descriptors[i].tint);
    decay.setComponentID("decayOrbit");
    decay.setTooltip("Reverb T60 decay (milliseconds). Click RENDER after changing.");

    capture.onClick=[this]{
        if(processor.isCapturing())processor.stopCapture();
        else {renderPending=false;processor.startCapture();}
    };
    render.onClick=[this]{processor.stopCapture();renderPending=true;};
    preview.onClick=[this]{
        if(processor.isPreviewPlaying())processor.stopPreview();
        else processor.startPreview();
    };
    save.onClick=[this]{saveWavDialog();};
    for(auto* btn:{&capture,&render,&preview,&save})addAndMakeVisible(*btn);
    capture.setColour(juce::TextButton::buttonColourId,violet);
    render.setColour(juce::TextButton::buttonColourId,royal);
    dragPad.fetchFile=[this]{return processor.getLatestWav();};
    addAndMakeVisible(dragPad);
    currentStatus=processor.getStatus();
    setSize(1390,840);
    startTimerHz(24);
}
DioVerbEditor::~DioVerbEditor() {
    stopTimer();
    chooser.reset();
    setLookAndFeel(nullptr);
}

void DioVerbEditor::setupKnob(juce::Slider& s,const juce::String& id,
                              std::unique_ptr<KnobLink>& link,juce::Colour color) {
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setRotaryParameters(juce::MathConstants<float>::pi*1.25f,
                          juce::MathConstants<float>::pi*2.75f,false);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,104,19);
    s.setMouseDragSensitivity(170);
    s.setPopupDisplayEnabled(true,false,this);
    s.setColour(juce::Slider::rotarySliderFillColourId,color);
    s.textFromValueFunction=[id](double v){return formatted(id,v);};
    s.valueFromTextFunction=[](const juce::String& value){
        const bool isKhz=value.containsIgnoreCase("kHz");
        return value.retainCharacters("-.0123456789").getDoubleValue()*
               (isKhz?1000.0:1.0);
    };
    if(auto* param=processor.parameters.getParameter(id))
        s.setDoubleClickReturnValue(true,param->convertFrom0to1(param->getDefaultValue()));
    addAndMakeVisible(s);
    link=std::make_unique<KnobLink>(processor.parameters,id,s);
}
void DioVerbEditor::timerCallback() {
    if(renderPending && !processor.isCaptureWriterBusy()) {
        renderPending=false;
        processor.renderCaptured();
    }
    currentStatus=processor.getStatus();
    runningPhase+=0.06f;
    meter=std::max(meter*0.87f,processor.getInputPeak());
    capture.setButtonText(processor.isCapturing()?"STOP CAPTURE":"CAPTURE");
    preview.setButtonText(processor.isPreviewPlaying()?"STOP PREVIEW":"PREVIEW");
    dragPad.repaint();
    repaint(25,678,1340,145);
}

void DioVerbEditor::paintWave(juce::Graphics& g,juce::Rectangle<float> a) {
    g.setColour(panel);
    g.fillRoundedRectangle(a,10.0f);
    g.setColour(royal.withAlpha(0.50f));
    g.drawRoundedRectangle(a,10.0f,1.5f);
    const bool ready=processor.hasRenderedAudio();
    const bool recording=processor.isCapturing();
    const float progress=processor.previewProgress();
    const float captureTime=float(processor.getCapturedFrames())/
        float(std::max(1.0,processor.getSampleRate()));
    for(int i=0;i<120;++i) {
        const float u=float(i)/119.0f;
        const float x=a.getX()+22.0f+u*(a.getWidth()-44.0f);
        float shape=std::pow(u,0.55f)*(0.33f+0.62f*
            std::abs(std::sin(u*59.0f)));
        if(!ready) shape=(recording?0.3f+meter:0.18f)*
            (0.30f+0.70f*std::abs(std::sin(u*65.0f+runningPhase)));
        const float half=(3.0f+shape*34.0f);
        g.setColour(ready&&u<progress?cyan:
                   ready?purple:juce::Colour(0xff464064));
        g.fillRoundedRectangle(juce::Rectangle<float>(
            x,a.getCentreY()-half,3.5f,half*2.0f),1.5f);
    }
    g.setColour(muted);
    font(g,11,true);
    g.drawText(recording?"RECORDING "+juce::String(captureTime,1)+"s / 12s":
              ready?"RENDERED REVERSE SWELL":"READY TO CAPTURE",
              int(a.getX())+19,int(a.getY())+7,370,19,
              juce::Justification::centredLeft);
}

void DioVerbEditor::paint(juce::Graphics& g) {
    g.fillAll(bg);
    g.setColour(juce::Colour(0xff100d21));
    g.fillRoundedRectangle(juce::Rectangle<float>(9,9,1372,822),14.0f);
    g.setColour(labelWhite);
    font(g,39.0f,true);
    g.drawText("Dio",30,18,90,55,juce::Justification::centredLeft);
    g.setColour(purple);
    g.drawText("VERB",103,18,190,55,juce::Justification::centredLeft);
    g.setColour(muted);
    font(g,12,true);
    g.drawText("REVERSE REVERB • ADVANCED SPACE ENGINE",316,37,548,26,
               juce::Justification::centredLeft);
    g.setColour(cyan);
    g.fillRoundedRectangle(juce::Rectangle<float>(1214,24,150,51),7.0f);
    g.setColour(bg);
    font(g,19,true);
    g.drawText(juce::String(juce::roundToInt(processor.getBpm()))+" BPM",
               1220,31,137,33,juce::Justification::centred);

    g.setColour(surface);
    g.fillRoundedRectangle(juce::Rectangle<float>(20,93,1350,76),10.0f);
    g.setColour(muted);
    font(g,11,true);
    g.drawText("SOURCE",28,101,167,17,juce::Justification::centredLeft);
    g.drawText("LENGTH",203,101,162,17,juce::Justification::centredLeft);

    panelBox(g,20,182,270,483,purple,"MASTER / DECAY");
    panelBox(g,301,182,220,483,violet,"DAMPING");
    panelBox(g,532,182,152,483,purple,"SHAPE");
    panelBox(g,695,182,153,483,royal,"DIFFUSION");
    panelBox(g,859,182,290,483,cyan,"MOD / MOTION");
    panelBox(g,1160,182,210,483,teal,"EQ / OUTPUT");

    label(g,"MIX",34,232,100,purple);
    label(g,"PREDELAY",165,232,112,purple);
    label(g,"DECAY TIME",65,350,179,purple);
    g.setColour(muted);
    font(g,10.5f);
    g.drawText("T60  /  milliseconds",60,608,189,20,
               juce::Justification::centred);

    label(g,"HIGH FREQ",309,237,103,violet);
    label(g,"HIGH SHELF",415,237,98,violet);
    label(g,"BASS FREQ",309,410,103,violet);
    label(g,"BASS MULT",415,410,98,violet);

    label(g,"SIZE",540,232,135,purple);
    label(g,"ATTACK",540,365,135,purple);
    label(g,"WIDTH",540,497,135,purple);

    label(g,"EARLY",700,232,140,royal);
    label(g,"LATE",700,365,140,royal);
    label(g,"TONE",700,497,140,royal);

    label(g,"RATE",870,232,126,cyan);
    label(g,"DEPTH",1011,232,126,cyan);
    g.setColour(darkStroke);
    g.drawHorizontalLine(364,873,1137);
    g.drawHorizontalLine(503,873,1137);
    g.setColour(cyan);
    font(g,12.0f,true);
    g.drawText("TREMOLO",875,373,132,19,juce::Justification::centredLeft);
    g.drawText("AUTOPAN",875,508,132,19,juce::Justification::centredLeft);
    label(g,"TREM DEPTH",1013,380,128,cyan);
    label(g,"PAN DEPTH",1013,516,128,cyan);

    label(g,"HIGH CUT",1167,232,197,teal);
    label(g,"LOW CUT",1167,365,197,teal);
    label(g,"OUTPUT",1167,497,197,teal);

    paintWave(g,{20,681,1350,92});
    g.setColour(juce::Colour(0xff151129));
    g.fillRoundedRectangle(juce::Rectangle<float>(20,781,1350,43),7.0f);
    g.setColour(processor.isCapturing()?purple:cyan);
    g.fillEllipse(36,796,10,10);
    g.setColour(labelWhite);
    font(g,12.0f);
    g.drawFittedText(currentStatus,58,789,1000,26,
                     juce::Justification::centredLeft,1);
    g.setColour(muted);
    font(g,10,true);
    g.drawText("DIOALEX MUSIC TOOLS  /  v0.3",1111,790,250,25,
               juce::Justification::centredRight);
}

void DioVerbEditor::resized() {
    mode.setBounds(28,122,166,34);
    length.setBounds(203,122,163,34);
    capture.setBounds(377,113,157,45);
    render.setBounds(546,113,155,45);
    preview.setBounds(713,113,157,45);
    save.setBounds(881,113,140,45);
    dragPad.setBounds(1033,113,322,45);

    mix.setBounds(33,254,105,96);
    predelay.setBounds(164,254,111,96);
    decay.setBounds(51,369,209,230);

    highFreq.setBounds(308,259,103,125);
    highShelf.setBounds(414,259,103,125);
    bassFreq.setBounds(308,430,103,125);
    bassMult.setBounds(414,430,103,125);

    size.setBounds(543,254,127,106);
    attack.setBounds(543,388,127,106);
    width.setBounds(543,520,127,106);

    early.setBounds(708,254,127,106);
    late.setBounds(708,388,127,106);
    tone.setBounds(708,520,127,106);

    modRate.setBounds(872,254,127,106);
    modDepth.setBounds(1010,254,127,106);
    tremolo.setBounds(874,405,127,31);
    tremDepth.setBounds(1010,399,127,102);
    autoPan.setBounds(874,538,127,31);
    panDepth.setBounds(1010,537,127,102);

    highCut.setBounds(1198,254,132,106);
    lowCut.setBounds(1198,388,132,106);
    output.setBounds(1198,520,132,106);
}

void DioVerbEditor::saveWavDialog() {
    const auto source=processor.getLatestWav();
    if(!source.existsAsFile())return;
    chooser=std::make_unique<juce::FileChooser>("Save DioVERB WAV",source,"*.wav");
    juce::Component::SafePointer<DioVerbEditor> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::saveMode |
                         juce::FileBrowserComponent::canSelectFiles,
       [safe,source](const juce::FileChooser& picker) {
           if(safe==nullptr)return;
           auto file=picker.getResult();
           if(file==juce::File())return;
           file=file.withFileExtension(".wav");
           if(file!=source)source.copyFileTo(file);
       });
}
