#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

namespace {
const juce::Colour bg(0xff111315);
const juce::Colour surface(0xff1c2023);
const juce::Colour pane(0xff252a2d);
const juce::Colour peach(0xffffb98b);
const juce::Colour light(0xfff8f2ea);
const juce::Colour muted(0xffa9adaf);
const juce::Colour teal(0xff77d7cf);
const juce::Colour gold(0xfff8d6a7);
const juce::Colour border(0xff40474a);
void type(juce::Graphics& g, float size, bool bold = false) {
    g.setFont(juce::Font(juce::FontOptions(size, bold
        ? juce::Font::bold : juce::Font::plain)));
}
void small(juce::Graphics& g, juce::String name, int x, int y, int w) {
    g.setColour(muted);
    type(g, 12.0f, true);
    g.drawText(name, x,y,w,18,juce::Justification::centred);
}
}

DioVerbLookAndFeel::DioVerbLookAndFeel() {
    setColour(juce::Slider::textBoxTextColourId, light);
    setColour(juce::Slider::textBoxBackgroundColourId, surface);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId, pane);
    setColour(juce::ComboBox::textColourId, light);
    setColour(juce::ComboBox::outlineColourId, border);
    setColour(juce::PopupMenu::backgroundColourId, pane);
    setColour(juce::PopupMenu::textColourId, light);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, peach);
    setColour(juce::PopupMenu::highlightedTextColourId, bg);
    setColour(juce::TextButton::buttonColourId, pane);
    setColour(juce::TextButton::textColourOffId, light);
}

void DioVerbLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y,
                                         int w, int h, float pos,
                                         float start, float end, juce::Slider&) {
    const float cx = x + w * 0.5f;
    const float cy = y + h * 0.47f;
    const float rad = float(std::min(w,h)) * 0.33f;
    g.setColour(juce::Colour(0xff0f1214));
    g.fillEllipse(cx-rad, cy-rad, 2*rad, 2*rad);
    juce::Path back, front;
    back.addCentredArc(cx,cy,rad+7,rad+7,0,start,end,true);
    front.addCentredArc(cx,cy,rad+7,rad+7,0,start,start+(end-start)*pos,true);
    g.setColour(juce::Colour(0xff545b5c));
    g.strokePath(back,juce::PathStrokeType(5.0f));
    g.setColour(peach);
    g.strokePath(front,juce::PathStrokeType(5.0f));
    g.setColour(juce::Colour(0xff303638));
    g.fillEllipse(cx-rad+6,cy-rad+6,2*rad-12,2*rad-12);
    const float theta=start+(end-start)*pos;
    g.setColour(light);
    g.drawLine(cx+std::sin(theta)*rad*0.3f,
               cy-std::cos(theta)*rad*0.3f,
               cx+std::sin(theta)*rad*0.80f,
               cy-std::cos(theta)*rad*0.80f,3.0f);
}

void DioVerbEditor::DragPad::paint(juce::Graphics& g) {
    const auto r=getLocalBounds().toFloat().reduced(1.0f);
    const bool ready = fetchFile && fetchFile().existsAsFile();
    g.setColour(ready ? juce::Colour(0xff7dc4b7) : juce::Colour(0xff545e5c));
    g.fillRoundedRectangle(r, 10.0f);
    g.setColour(bg);
    type(g, 19.0f, true);
    g.drawText(ready ? "⇢  DRAG WAV TO DAW" : "DRAG WAV  /  RENDER FIRST",
               getLocalBounds().reduced(7), juce::Justification::centred);
}
void DioVerbEditor::DragPad::mouseDown(const juce::MouseEvent&) {
    armed = true;
}
void DioVerbEditor::DragPad::mouseDrag(const juce::MouseEvent& e) {
    if (!armed || e.getDistanceFromDragStart() < 8) return;
    armed = false;
    if (fetchFile) {
        const auto file = fetchFile();
        if (file.existsAsFile()) {
            juce::DragAndDropContainer::performExternalDragDropOfFiles(
                juce::StringArray {file.getFullPathName()}, false, this);
        }
    }
}

DioVerbEditor::DioVerbEditor(DioVerbProcessor& p)
    : juce::AudioProcessorEditor(&p), processor(p) {
    setLookAndFeel(&look);
    mode.addItemList({"VOCAL", "INSTRUMENT"}, 1);
    length.addItemList({"1/4 BAR", "1/2 BAR", "1 BAR"}, 1);
    tremolo.addItemList({"OFF", "1/8", "1/16", "1/32"}, 1);
    auto& ps=processor.parameters;
    choiceLinks[0]=std::make_unique<ChoiceLink>(ps, "mode", mode);
    choiceLinks[1]=std::make_unique<ChoiceLink>(ps, "length", length);
    choiceLinks[2]=std::make_unique<ChoiceLink>(ps, "tremolo", tremolo);
    addAndMakeVisible(mode);
    addAndMakeVisible(length);
    addAndMakeVisible(tremolo);

    setupKnob(size, "size", knobLinks[0]);
    setupKnob(tone, "tone", knobLinks[1]);
    setupKnob(width, "width", knobLinks[2]);
    setupKnob(depth, "depth", knobLinks[3]);
    setupKnob(output, "output", knobLinks[4]);

    capture.onClick=[this] {
        if (processor.isCapturing()) processor.stopCapture();
        else {
            renderPending=false;
            processor.startCapture();
        }
    };
    render.onClick=[this] {
        processor.stopCapture();
        renderPending=true;
    };
    preview.onClick=[this] {
        if (processor.isPreviewPlaying()) processor.stopPreview();
        else processor.startPreview();
    };
    save.onClick=[this]{ saveWavDialog(); };
    for (auto* b : {&capture,&render,&preview,&save}) addAndMakeVisible(*b);
    capture.setColour(juce::TextButton::buttonColourId, peach);
    capture.setColour(juce::TextButton::textColourOffId, bg);
    render.setColour(juce::TextButton::buttonColourId, gold);
    render.setColour(juce::TextButton::textColourOffId, bg);

    dragPad.fetchFile=[this]{return processor.getLatestWav();};
    addAndMakeVisible(dragPad);
    currentStatus=processor.getStatus();
    setSize(1090, 690);
    startTimerHz(24);
}

DioVerbEditor::~DioVerbEditor() {
    stopTimer();
    chooser.reset();
    setLookAndFeel(nullptr);
}
void DioVerbEditor::setupKnob(
    juce::Slider& s, const juce::String& id, std::unique_ptr<KnobLink>& link) {
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setRotaryParameters(juce::MathConstants<float>::pi*1.25f,
                          juce::MathConstants<float>::pi*2.75f,false);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,84,21);
    s.setPopupDisplayEnabled(true,false,this);
    s.setMouseDragSensitivity(180);
    s.textFromValueFunction = [id](double value) {
        if(id=="output")
            return (value>0.0?"+":"") + juce::String(value,1) + " dB";
        return juce::String(juce::roundToInt(value)) + "%";
    };
    s.valueFromTextFunction=[](const juce::String& str) {
        return str.retainCharacters("-.0123456789").getDoubleValue();
    };
    if (auto* param=processor.parameters.getParameter(id))
        s.setDoubleClickReturnValue(true,
            param->convertFrom0to1(param->getDefaultValue()));
    addAndMakeVisible(s);
    link=std::make_unique<KnobLink>(processor.parameters,id,s);
}

void DioVerbEditor::timerCallback() {
    if (renderPending && !processor.isCaptureWriterBusy()) {
        renderPending=false;
        processor.renderCaptured();
    }
    currentStatus=processor.getStatus();
    runningPhase+=0.045f;
    meter=std::max(processor.getInputPeak(),meter*0.89f);
    capture.setButtonText(processor.isCapturing()?"STOP CAPTURE":"CAPTURE");
    preview.setButtonText(processor.isPreviewPlaying()?"STOP PREVIEW":"PREVIEW");
    dragPad.repaint();
    repaint(30, 219, 1030, 174);
    repaint(30, 600, 1030, 80);
}

void DioVerbEditor::paintWave(juce::Graphics& g, juce::Rectangle<float> bounds) {
    g.setColour(juce::Colour(0xff15191b));
    g.fillRoundedRectangle(bounds,12.0f);
    g.setColour(juce::Colour(0xff293134));
    for (int i=1;i<6;++i)
        g.drawVerticalLine(juce::roundToInt(bounds.getX()+i*bounds.getWidth()/6.0f),
                           bounds.getY()+16,bounds.getBottom()-16);
    g.setColour(juce::Colour(0xff3e484a));
    g.drawHorizontalLine(juce::roundToInt(bounds.getCentreY()),
                         bounds.getX()+14,bounds.getRight()-14);

    const bool ready=processor.hasRenderedAudio();
    const float progress=processor.previewProgress();
    const float captureMs=1000.0f*float(processor.getCapturedFrames()) /
                         float(std::max(1.0,processor.getSampleRate()));
    const bool capturing=processor.isCapturing();
    const int bars=125;
    const float stride=(bounds.getWidth()-34.0f)/float(bars);
    for (int i=0;i<bars;++i) {
        const float u=float(i)/float(bars-1);
        float env=std::pow(u,0.68f)*(0.35f+0.50f*
                  std::abs(std::sin(u*76.0f+0.2f)));
        if(!ready) env=(capturing ? 0.35f+meter : 0.22f)*
                  (0.3f+0.7f*std::abs(std::sin(u*54.0f+runningPhase)));
        const float height=3.0f+env*(bounds.getHeight()*0.36f);
        const float x=bounds.getX()+17.0f+float(i)*stride;
        const bool heard=ready && u <= progress;
        g.setColour(heard ? teal : (ready ? peach : juce::Colour(0xff636f70)));
        g.fillRoundedRectangle(juce::Rectangle<float>(
            x,bounds.getCentreY()-height,2.8f,2.0f*height),1.2f);
    }
    if (ready && processor.isPreviewPlaying()) {
        const float x=bounds.getX()+17.0f+progress*(bounds.getWidth()-34.0f);
        g.setColour(light);
        g.drawVerticalLine(juce::roundToInt(x),bounds.getY()+6,bounds.getBottom()-6);
    }
    g.setColour(muted);
    type(g,11.0f,true);
    g.drawText(ready?"RENDERED REVERSE SWELL":
               (capturing?"LIVE CAPTURE":"REVERSE SWELL PREVIEW"),
               juce::Rectangle<int>(int(bounds.getX())+17,int(bounds.getY())+12,310,22),
               juce::Justification::centredLeft);
    if (capturing) {
        g.setColour(peach);
        g.drawText(juce::String(captureMs/1000.0f,1)+" s / 12 s",
                   juce::Rectangle<int>(int(bounds.getRight())-188,
                                        int(bounds.getY())+12,171,22),
                   juce::Justification::centredRight);
    }
}

void DioVerbEditor::paint(juce::Graphics& g) {
    g.fillAll(bg);
    g.setColour(surface);
    g.fillRoundedRectangle(juce::Rectangle<float>(13,12,1064,665),14.0f);
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xfff6c5a2),22,24,
        juce::Colour(0xffc87965),1067,130,false));
    g.fillRoundedRectangle(juce::Rectangle<float>(22,20,1046,124),10.0f);
    g.setColour(bg);
    type(g,56.0f,true);
    g.drawText("DioVERB",43,27,429,72,juce::Justification::centredLeft);
    type(g,12.8f,true);
    g.drawText("REVERSE REVERB  /  TIME-SYNCED RISING SPACE",
               48,104,544,22,juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff322323));
    type(g,12.0f,true);
    g.drawText("CAPTURE   •   SHAPE   •   PREVIEW   •   DRAG",
               620,56,418,27,juce::Justification::centredRight);

    paintWave(g,{29,219,1032,174});

    const float bpm=processor.getBpm();
    dioverb::Settings timing;
    timing.sampleRate=processor.getSampleRate();
    timing.bpm=bpm;
    const int idx=int(processor.parameters.getRawParameterValue("length")->load());
    timing.length=static_cast<dioverb::Length>(std::clamp(idx,0,2));
    const float durationMs=1000.0f * float(dioverb::lengthInSamples(timing)) /
                           float(std::max(1.0,timing.sampleRate));
    g.setColour(peach);
    type(g,13.0f,true);
    g.drawText(juce::String(juce::roundToInt(bpm))+" BPM",
               856,158,198,24,juce::Justification::centredRight);
    g.setColour(muted);
    type(g,11.0f,true);
    g.drawText("SOURCE",43,149,204,22,juce::Justification::centredLeft);
    g.drawText("LENGTH",268,149,204,22,juce::Justification::centredLeft);
    g.drawText(juce::String(juce::roundToInt(durationMs))+" ms",
               480,154,170,22,juce::Justification::centredLeft);

    g.setColour(pane);
    g.fillRoundedRectangle(juce::Rectangle<float>(29,466,1032,125),11.0f);
    constexpr const char* labels[]={"SIZE","TONE","WIDTH","DEPTH","OUTPUT"};
    for(int i=0;i<5;++i)small(g,labels[i],66+i*185,566,126);
    g.setColour(muted);
    type(g,10.8f,true);
    g.drawText("TREMOLO",935,476,118,18,juce::Justification::centred);

    const auto statusPanel=juce::Rectangle<float>(29,598,1032,67);
    g.setColour(juce::Colour(0xff161b1c));
    g.fillRoundedRectangle(statusPanel,9.0f);
    g.setColour(processor.isCapturing()?peach:teal);
    g.fillEllipse(46,613,9,9);
    g.setColour(light);
    type(g,12.0f);
    g.drawFittedText(currentStatus,65,607,643,24,
                     juce::Justification::centredLeft,1);

    // Input meter (capture) - not a master loudness meter.
    g.setColour(muted);
    type(g,9.0f,true);
    g.drawText("INPUT",876,607,158,14,juce::Justification::centred);
    for (int i=0;i<16;++i) {
        g.setColour(i/16.0f < meter ? (i>12?peach:teal) : border);
        g.fillRoundedRectangle(juce::Rectangle<float>(887+i*9,627,6.2f,11),1.0f);
    }
    g.setColour(muted);
    type(g,10.0f);
    g.drawText("© DIOALEX MUSIC TOOLS  •  ORIGINAL AUDIO ENGINE  •  v0.1",
               39,664,1005,14,juce::Justification::centredRight);
}

void DioVerbEditor::resized() {
    mode.setBounds(43,176,190,31);
    length.setBounds(268,176,190,31);
    auto rows=std::array<juce::Slider*,5>{
        &size,&tone,&width,&depth,&output
    };
    for(int i=0;i<5;++i)rows[size_t(i)]->setBounds(68+i*185,473,126,92);
    tremolo.setBounds(935,505,117,30);
    // Action row above waveform, always accessible.
    capture.setBounds(43,400,204,49);
    render.setBounds(261,400,199,49);
    preview.setBounds(473,400,196,49);
    save.setBounds(684,400,148,49);
    dragPad.setBounds(844,400,202,49);
}
void DioVerbEditor::saveWavDialog() {
    const auto source=processor.getLatestWav();
    if (!source.existsAsFile()) return;
    chooser=std::make_unique<juce::FileChooser>(
        "Save DioVERB WAV",source,"*.wav");
    juce::Component::SafePointer<DioVerbEditor> safe(this);
    chooser->launchAsync(
        juce::FileBrowserComponent::saveMode |
        juce::FileBrowserComponent::canSelectFiles,
        [safe,source](const juce::FileChooser& picker) {
            if (!safe) return;
            auto selected=picker.getResult();
            if (selected==juce::File()) return;
            selected=selected.withFileExtension(".wav");
            if (selected==source) return;
            source.copyFileTo(selected);
        });
}
