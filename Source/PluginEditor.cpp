#include "PluginEditor.h"
#include "FactoryPresets.h"
#include "Timing.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace {
const juce::Colour lime(0xff24c6fa);
const juce::Colour ink(0xff070713);
const juce::Colour background(0xff0d0b1d);
const juce::Colour panel(0xff18132d);
const juce::Colour card(0xff151126);
const juce::Colour pale(0xfff7f5ff);
const juce::Colour muted(0xffaba5cb);
const juce::Colour violet(0xff9560ff);
constexpr std::array<const char*, 6> names {
    "NOISE", "WOBBLE", "DISTORT", "DIGITAL", "SPACE", "MAGNETIC"
};
constexpr std::array<const char*, 6> subtitles {
    "ANALOG TEXTURE", "TAPE MOVEMENT", "HARMONIC COLOR",
    "SAMPLE TEXTURE", "REVERB SPACE", "TAPE WEAR"
};
constexpr std::array<const char*, 6> amountIds {
    "noise", "wobble", "distort", "digital", "space", "magnetic"
};
constexpr std::array<const char*, 6> characterIds {
    "noiseColor", "wobbleRate", "distortionTone", "digitalRate",
    "spaceSize", "magneticSpeed"
};
constexpr std::array<const char*, 6> characterLabels {
    "COLOR", "RATE", "TONE", "CRUSH", "DECAY", "SPEED"
};
constexpr std::array<const char*, 6> fluxIds {
    "noiseFlux", "wobbleFlux", "distortFlux",
    "digitalFlux", "spaceFlux", "magneticFlux"
};
constexpr std::array<const char*, 6> enables {
    "noiseOn", "wobbleOn", "distortOn", "digitalOn", "spaceOn", "magneticOn"
};
const std::array<juce::Colour, 6> accent {
    juce::Colour(0xff9862ff), juce::Colour(0xff4e8dff),
    juce::Colour(0xffbe4bff), juce::Colour(0xff25c9fa),
    juce::Colour(0xff6685ff), juce::Colour(0xff1ec5dc)
};

void font(juce::Graphics& g, float size, bool bold = false) {
    g.setFont(juce::Font(juce::FontOptions(size, bold
               ? juce::Font::bold : juce::Font::plain)));
}
// One formatting function is shared by every slider readout. Trailing zeros
// are removed, while two decimal places remain the strict upper limit.
juce::String exactNumber(double value) {
    auto result = juce::String(value, 2);
    while (result.containsChar('.') && result.endsWithChar('0'))
        result = result.dropLastCharacters(1);
    if (result.endsWithChar('.'))
        result = result.dropLastCharacters(1);
    return result;
}
juce::String valueLabel(const juce::String& id, double value) {
    if (id == "lowCut" || id == "highCut" || id == "wobbleRate")
        return exactNumber(value) + " Hz";
    if (id == "spaceSize" || id == "magneticSpeed")
        return exactNumber(value) + " ms";
    if (id == "outputDb" || id == "distortionTone")
        return (value > 0.001 ? "+" : "") + exactNumber(value) + " dB";
    return exactNumber(value) + "%";
}
}

void DC23SettingsGear::paintButton(juce::Graphics& g, bool hovered, bool down) {
    const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(down ? juce::Colour(0xff42336d)
                     : hovered ? juce::Colour(0xff2c2550) : panel);
    g.fillRoundedRectangle(bounds, 8.0f);
    g.setColour(violet);
    g.drawRoundedRectangle(bounds, 8.0f, 1.2f);
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    for (int i = 0; i < 8; ++i) {
        const float angle = juce::MathConstants<float>::twoPi * float(i) / 8.0f;
        g.drawLine(cx + std::cos(angle) * 8.5f,
                   cy + std::sin(angle) * 8.5f,
                   cx + std::cos(angle) * 12.8f,
                   cy + std::sin(angle) * 12.8f, 3.1f);
    }
    g.drawEllipse(cx - 8.5f, cy - 8.5f, 17.0f, 17.0f, 3.0f);
    g.fillEllipse(cx - 2.4f, cy - 2.4f, 4.8f, 4.8f);
}

DC23SettingsPanel::DC23SettingsPanel() {
    scaleControl.setSliderStyle(juce::Slider::LinearHorizontal);
    scaleControl.setRange(50.0, 200.0, 1.0);
    scaleControl.setTextBoxStyle(juce::Slider::TextBoxRight, false, 78, 24);
    scaleControl.setColour(juce::Slider::trackColourId, violet);
    scaleControl.setColour(juce::Slider::thumbColourId, lime);
    scaleControl.textFromValueFunction = [](double value) {
        return juce::String(juce::roundToInt(value)) + "%";
    };
    scaleControl.valueFromTextFunction = [](const juce::String& text) {
        return text.retainCharacters("0123456789.-").getDoubleValue();
    };
    scaleControl.setDoubleClickReturnValue(true, 100.0);
    addAndMakeVisible(scaleControl);
    addAndMakeVisible(aboutButton);
}
void DC23SettingsPanel::paint(juce::Graphics& g) {
    const float scale = float(getWidth()) / 345.0f;
    g.addTransform(juce::AffineTransform::scale(scale));
    g.setColour(juce::Colour(0xff111024));
    g.fillRoundedRectangle(juce::Rectangle<float>(1, 1, 343, 178), 12.0f);
    g.setColour(violet);
    g.drawRoundedRectangle(juce::Rectangle<float>(1, 1, 343, 178), 12.0f, 2.0f);
    g.setColour(pale);
    font(g, 17.0f, true);
    g.drawText("SETTINGS", 18, 12, 300, 30, juce::Justification::centredLeft);
    g.setColour(muted);
    font(g, 12.0f);
    g.drawText("SCALE  (50% – 200%)", 18, 48, 300, 22,
               juce::Justification::centredLeft);
    g.drawText("Resize the DC-23 interface", 18, 119, 305, 23,
               juce::Justification::centredLeft);
}
void DC23SettingsPanel::resized() {
    const float scale = float(getWidth()) / 345.0f;
    auto sb = [scale](int x, int y, int w, int h) {
        return juce::Rectangle<int>(juce::roundToInt(x * scale),
                                    juce::roundToInt(y * scale),
                                    juce::roundToInt(w * scale),
                                    juce::roundToInt(h * scale));
    };
    scaleControl.setBounds(sb(18, 73, 306, 35));
    aboutButton.setBounds(sb(189, 143, 135, 28));
}

DC23LookAndFeel::DC23LookAndFeel() {
    setColour(juce::Slider::textBoxTextColourId, pale);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff0b0a1c));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId, panel);
    setColour(juce::ComboBox::textColourId, pale);
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff49416a));
    setColour(juce::PopupMenu::backgroundColourId, panel);
    setColour(juce::PopupMenu::textColourId, pale);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, lime);
    setColour(juce::PopupMenu::highlightedTextColourId, ink);
    setColour(juce::TextButton::buttonColourId, card);
    setColour(juce::TextButton::textColourOffId, pale);
}

void DC23LookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                                      float position, float start, float end,
                                      juce::Slider& slider) {
    const float size = float(std::min(w, h)) * 0.77f;
    const float cx = float(x) + float(w) * 0.5f;
    const float cy = float(y) + float(h) * 0.44f;
    const float radius = size * 0.43f;
    const bool bipolar = slider.getComponentID() == "bipolar";
    const auto accentColour = slider.findColour(juce::Slider::rotarySliderFillColourId, true);
    g.setColour(juce::Colour(0xff090a19));
    g.fillEllipse(cx-radius, cy-radius, radius*2.0f, radius*2.0f);

    juce::Path back, active;
    back.addCentredArc(cx,cy,radius+6.0f,radius+6.0f,0,start,end,true);
    const float angle = start + (end-start) * position;
    if (bipolar) {
        const float center = (start+end)*0.5f;
        if (position >= 0.5f)
            active.addCentredArc(cx,cy,radius+6.0f,radius+6.0f,0,center,angle,true);
        else
            active.addCentredArc(cx,cy,radius+6.0f,radius+6.0f,0,angle,center,true);
    } else {
        active.addCentredArc(cx,cy,radius+6.0f,radius+6.0f,0,start,angle,true);
    }
    g.setColour(juce::Colour(0xff3a3556));
    g.strokePath(back, juce::PathStrokeType(4.5f));
    if (!bipolar || std::abs(position-0.5f) > 0.0001f) {
        g.setColour(bipolar && position < 0.5f ? violet : accentColour);
        g.strokePath(active, juce::PathStrokeType(4.8f));
    }

    g.setColour(juce::Colour(0xff24203d));
    g.fillEllipse(cx-radius+6.0f, cy-radius+6.0f,
                  radius*2.0f-12.0f, radius*2.0f-12.0f);
    g.setColour(juce::Colour(0xff69678f));
    g.drawEllipse(cx-radius+1.0f, cy-radius+1.0f,
                  radius*2.0f-2.0f, radius*2.0f-2.0f, 1.0f);
    g.setColour(bipolar ? lime : pale);
    g.drawLine(cx + std::sin(angle) * radius * 0.20f,
               cy - std::cos(angle) * radius * 0.20f,
               cx + std::sin(angle) * radius * 0.74f,
               cy - std::cos(angle) * radius * 0.74f, 3.0f);
    if (bipolar) {
        g.setColour(lime.withAlpha(0.8f));
        g.fillEllipse(cx-2.3f,cy-radius-10.2f,4.6f,4.6f);
    }
}

void DC23LookAndFeel::drawToggleButton(juce::Graphics& g,
                                      juce::ToggleButton& button, bool, bool) {
    const auto b = button.getLocalBounds().toFloat().reduced(2.0f);
    const bool on = button.getToggleState();
    g.setColour(on ? lime : juce::Colour(0xff40395c));
    g.fillRoundedRectangle(b, 5.0f);
    g.setColour(ink);
    font(g, 11.0f, true);
    g.drawText(on ? "ON" : "OFF", button.getLocalBounds(),
               juce::Justification::centred);
}

juce::Rectangle<int> DC23AudioProcessorEditor::moduleBounds(int index) {
    return {23 + index * 219, 168, 208, 390};
}

DC23AudioProcessorEditor::DC23AudioProcessorEditor(DC23AudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p) {
    setLookAndFeel(&look);
    for (size_t i = 0; i < modules.size(); ++i) {
        auto& m = modules[i];
        setupKnob(m.amount, amountIds[i], m.amountLink);
        setupKnob(m.character, characterIds[i], m.characterLink);
        setupKnob(m.flux, fluxIds[i], m.fluxLink);
        m.amount.setColour(juce::Slider::rotarySliderFillColourId, accent[i]);
        m.character.setColour(juce::Slider::rotarySliderFillColourId, accent[i]);
        m.flux.setColour(juce::Slider::rotarySliderFillColourId, violet);
        m.amount.setTooltip(juce::String(names[i]) + " amount: 0–100%");
        m.flux.setTooltip("Organic modulation: 0–100% Flux");
        m.enabled.setClickingTogglesState(true);
        addAndMakeVisible(m.enabled);
        m.enabledLink = std::make_unique<ButtonLink>(processor.parameters,
                                                      enables[i], m.enabled);
    }
    setupKnob(magnitude, "magnitude", masterLinks[0]);
    setupKnob(mix, "mix", masterLinks[1]);
    setupKnob(output, "outputDb", masterLinks[2]);
    setupKnob(echoMix, "echoMix", masterLinks[3]);
    setupKnob(echoFeedback, "echoFeedback", masterLinks[4]);
    setupKnob(width, "width", masterLinks[5]);
    setupKnob(lowCut, "lowCut", masterLinks[6]);
    setupKnob(highCut, "highCut", masterLinks[7]);
    setupKnob(digitalSmooth, "digitalSmooth", masterLinks[8]);

    digitalSmooth.setSliderStyle(juce::Slider::LinearHorizontal);
    digitalSmooth.setTextBoxStyle(juce::Slider::TextBoxRight, false, 57, 18);
    digitalSmooth.setTooltip("Smoothes the digital rate and bit degradation");

    for (auto* button : {&echoEnabled, &lowEnabled, &highEnabled}) {
        button->setClickingTogglesState(true);
        addAndMakeVisible(*button);
    }
    echoEnabledLink = std::make_unique<ButtonLink>(processor.parameters, "echoOn", echoEnabled);
    lowEnabledLink = std::make_unique<ButtonLink>(processor.parameters, "lowCutOn", lowEnabled);
    highEnabledLink = std::make_unique<ButtonLink>(processor.parameters, "highCutOn", highEnabled);

    noiseStyle.addItemList(juce::StringArray{"TAPE","VINYL","HUM"}, 1);
    distortionStyle.addItemList(juce::StringArray{"TAPE","VALVE","FUZZ"}, 1);
    noiseStyleLink = std::make_unique<ComboLink>(processor.parameters,
                                                "noiseStyle", noiseStyle);
    distortionStyleLink = std::make_unique<ComboLink>(processor.parameters,
                                                     "distortionStyle", distortionStyle);
    addAndMakeVisible(noiseStyle);
    addAndMakeVisible(distortionStyle);

    for (size_t i = 0; i < dc23::noteDivisions.size(); ++i)
        echoDivision.addItem(dc23::noteDivisions[i].label, int(i)+1);
    echoDivisionLink = std::make_unique<ComboLink>(processor.parameters,
                                                  "echoDivision", echoDivision);
    addAndMakeVisible(echoDivision);

    factoryPresets.setTextWhenNothingSelected("CHOOSE A VINTAGE COLOR");
    const auto& presets = dc23::factoryPresets();
    for (size_t i = 0; i < presets.size(); ++i) {
        factoryPresets.addItem(juce::String(presets[i].category) + "  /  " +
                               presets[i].name, int(i)+1);
    }
    factoryPresets.onChange = [this] {
        const int id = factoryPresets.getSelectedId();
        if (id > 0) processor.applyFactoryPreset(id-1);
    };
    addAndMakeVisible(factoryPresets);
    saveButton.onClick = [this] { openPreset(true); };
    loadButton.onClick = [this] { openPreset(false); };
    addAndMakeVisible(saveButton);
    addAndMakeVisible(loadButton);

    settingsGear.onClick = [this] {
        settingsOpen = !settingsOpen;
        settingsPanel.setVisible(settingsOpen);
        settingsPanel.toFront(true);
        repaint(scaledBounds(960, 154, 360, 193));
    };
    addAndMakeVisible(settingsGear);
    settingsPanel.scaleControl.onValueChange = [this] {
        setInterfaceScale(juce::roundToInt(settingsPanel.scaleControl.getValue()), true);
    };
    settingsPanel.aboutButton.onClick = [this] { showAbout(); };
    addAndMakeVisible(settingsPanel);
    settingsPanel.setVisible(false);
    scalePercent = juce::jlimit(50, 200,
        int(processor.parameters.state.getProperty("uiScalePercent", 100)));
    settingsPanel.scaleControl.setValue(double(scalePercent),
                                        juce::dontSendNotification);
    setSize(juce::roundToInt(1360.0f * float(scalePercent) / 100.0f),
            juce::roundToInt(799.0f * float(scalePercent) / 100.0f));
    startTimerHz(30);
}

DC23AudioProcessorEditor::~DC23AudioProcessorEditor() {
    stopTimer();
    chooser.reset();
    setLookAndFeel(nullptr);
}

juce::Rectangle<int> DC23AudioProcessorEditor::scaledBounds(
        int x, int y, int w, int h) const {
    const float factor = float(scalePercent) * 0.01f;
    return { juce::roundToInt(float(x) * factor),
             juce::roundToInt(float(y) * factor),
             juce::roundToInt(float(w) * factor),
             juce::roundToInt(float(h) * factor) };
}
void DC23AudioProcessorEditor::setInterfaceScale(int percent, bool saveSetting) {
    percent = juce::jlimit(50, 200, percent);
    if (saveSetting)
        processor.parameters.state.setProperty("uiScalePercent", percent, nullptr);
    if (scalePercent != percent) {
        scalePercent = percent;
        setSize(juce::roundToInt(1360.0f * float(percent) / 100.0f),
                juce::roundToInt(799.0f * float(percent) / 100.0f));
        repaint();
    }
}
void DC23AudioProcessorEditor::showAbout() {
    juce::AlertWindow::showMessageBoxAsync(
        juce::MessageBoxIconType::InfoIcon,
        "About DC-23",
        "DC-23  |  Version 1.0\nVintage Color Lab\nDeveloped by Dioalex Music Tools",
        "OK", this);
}

void DC23AudioProcessorEditor::setupKnob(
    juce::Slider& slider, const juce::String& id,
    std::unique_ptr<SliderLink>& attachment, bool bipolar) {
    // Circular controls with an explicit stop, not wrapping from max to min.
    slider.setSliderStyle(juce::Slider::Rotary);
    slider.setRotaryParameters(juce::MathConstants<float>::pi*1.25f,
                               juce::MathConstants<float>::pi*2.75f, true);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 94, 20);
    slider.setMouseDragSensitivity(180);
    slider.setPopupDisplayEnabled(true, false, this);
    slider.setColour(juce::Slider::rotarySliderFillColourId, lime);
    if (bipolar) slider.setComponentID("bipolar");
    slider.textFromValueFunction = [id](double v) { return valueLabel(id, v); };
    slider.valueFromTextFunction = [](const juce::String& text) {
        const bool khz = text.containsIgnoreCase("khz");
        return text.retainCharacters("0123456789.-").getDoubleValue() * (khz ? 1000.0 : 1.0);
    };
    if (auto* parameter = processor.parameters.getParameter(id)) {
        slider.setDoubleClickReturnValue(true,
              parameter->convertFrom0to1(parameter->getDefaultValue()));
    }
    addAndMakeVisible(slider);
    attachment = std::make_unique<SliderLink>(processor.parameters, id, slider);
}

void DC23AudioProcessorEditor::timerCallback() {
    animation += 0.105f;
    if (animation >= 1000.0f) animation -= 1000.0f;
    meterDisplay = std::max(processor.getOutputPeak(), meterDisplay * 0.86f);
    const int saved = juce::jlimit(50, 200,
        int(processor.parameters.state.getProperty("uiScalePercent", 100)));
    if (saved != scalePercent) {
        settingsPanel.scaleControl.setValue(double(saved),
                                            juce::dontSendNotification);
        setInterfaceScale(saved, false);
    }
    repaint(scaledBounds(20, 212, 1320, 87));  // animated traces
    repaint(scaledBounds(1100, 80, 208, 47)); // output meter
    repaint(scaledBounds(210, 714, 200, 60)); // Echo BPM
}

void DC23AudioProcessorEditor::drawModulePlot(juce::Graphics& g, int index,
                                               juce::Rectangle<float> box) {
    g.setColour(juce::Colour(0xff0a0b1a));
    g.fillRoundedRectangle(box, 6.0f);
    g.setColour(juce::Colour(0xff2e2a50));
    for (int y = 1; y < 4; ++y) {
        const float pos = box.getY() + box.getHeight() * float(y) / 4.0f;
        g.drawHorizontalLine(juce::roundToInt(pos),
                             box.getX()+4, box.getRight()-4);
    }
    const auto* amountParam = processor.parameters.getRawParameterValue(amountIds[size_t(index)]);
    const auto* fluxParam = processor.parameters.getRawParameterValue(fluxIds[size_t(index)]);
    const auto* enableParam = processor.parameters.getRawParameterValue(enables[size_t(index)]);
    const float amount = amountParam ? amountParam->load() * 0.01f : 0.0f;
    const float flux = fluxParam ? fluxParam->load() * 0.01f : 0.0f;
    const bool enabled = enableParam && enableParam->load() > 0.5f;
    const float motion = animation * (0.45f + 0.4f*flux);
    const float scale = (5.0f + 16.0f*amount + 7.0f*std::min(meterDisplay, 1.0f))
                        * (enabled ? 1.0f : 0.12f);

    juce::Path trace, shadow;
    constexpr int steps = 100;
    for (int k = 0; k <= steps; ++k) {
        const float u = float(k)/float(steps);
        const float t = u * 16.0f;
        float value = 0.0f;
        switch(index) {
            case 0: value = std::sin(t*7.7f+motion*2.2f)*0.30f +
                             std::sin(t*3.3f-motion)*0.23f; break;
            case 1: value = std::sin(t*0.75f+motion) *
                             (0.40f+0.14f*std::sin(t*2.1f+motion)); break;
            case 2: value = std::tanh(2.4f*std::sin(t*1.25f+motion)); break;
            case 3: value = std::round(std::sin(t*1.3f+motion)*3.0f)/3.0f; break;
            case 4: value = std::sin(t*1.15f+motion) *
                             std::exp(-0.065f*t) + 0.1f*std::sin(t*2.1f-motion); break;
            default: value = std::sin(t*1.9f+motion) *
                             (0.35f+0.65f*std::abs(std::sin(t*0.19f+motion*0.7f))); break;
        }
        value += flux * 0.16f * std::sin(t*2.7f-motion*1.6f);
        const float x = box.getX()+5.0f + u*(box.getWidth()-10.0f);
        const float y = box.getCentreY()-value*scale;
        if (k == 0) {
            trace.startNewSubPath(x,y);
            shadow.startNewSubPath(x,y+3.0f);
        } else {
            trace.lineTo(x,y);
            shadow.lineTo(x,y+3.0f);
        }
    }
    g.setColour(accent[size_t(index)].withAlpha(enabled ? 0.28f : 0.09f));
    g.strokePath(shadow, juce::PathStrokeType(5.0f));
    g.setColour(accent[size_t(index)].withAlpha(enabled ? 0.95f : 0.25f));
    g.strokePath(trace, juce::PathStrokeType(2.0f));
    g.setColour(muted.withAlpha(0.6f));
    font(g, 8.0f, true);
    g.drawText("SIGNAL MOTION", box.toNearestInt().reduced(7),
               juce::Justification::topLeft);
}

void DC23AudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(ink);
    g.addTransform(juce::AffineTransform::scale(float(scalePercent) * 0.01f));
    g.setColour(background);
    g.fillRoundedRectangle(juce::Rectangle<float>(12, 11, 1336, 777), 13.0f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff160f31), 27.0f, 20.0f,
                      juce::Colour(0xff10132b), 1331.0f, 151.0f, false));
    g.fillRoundedRectangle(juce::Rectangle<float>(20, 18, 1320, 135), 10.0f);
    g.setColour(violet);
    g.drawRoundedRectangle(juce::Rectangle<float>(20, 18, 1320, 135), 10.0f, 1.3f);
    g.setColour(pale);
    font(g, 67, true);
    g.drawText("DC-", 36, 26, 154, 85, juce::Justification::centredLeft);
    g.setColour(violet);
    g.drawText("23", 162, 26, 144, 85, juce::Justification::centredLeft);
    g.setColour(muted);
    font(g, 13.5f, true);
    g.drawText("VINTAGE COLOR LAB  /  v1.0", 43, 112, 320, 22,
               juce::Justification::centredLeft);
    g.setColour(pale);
    font(g, 11.0f, true);
    constexpr std::array<const char*, 3> topLabels {
        "MAGNITUDE", "DRY / WET", "OUTPUT"
    };
    for (int i = 0; i < 3; ++i)
        g.drawText(topLabels[size_t(i)], 364 + i*139, 126, 116, 18,
                   juce::Justification::centred);
    g.drawText("FACTORY COLORS", 818, 31, 275, 19,
               juce::Justification::centredLeft);
    g.drawText("0% DRY    /    50% BLEND    /    100% WET", 474, 20, 330, 16,
               juce::Justification::centred);

    // Audio-reactive meter in the header.
    g.setColour(juce::Colour(0xff0b0b1b));
    g.fillRoundedRectangle(juce::Rectangle<float>(1104, 99, 216, 33), 6.0f);
    for (int s = 0; s < 24; ++s) {
        const float x = 1113.0f + float(s)*8.2f;
        const float level = float(s)/24.0f;
        g.setColour(level <= meterDisplay ? (level > 0.83f ? juce::Colour(0xffba58ff) : lime)
                         : juce::Colour(0xff3d3559));
        g.fillRoundedRectangle(juce::Rectangle<float>(x, 107, 5.8f, 16), 1.0f);
    }

    for (int i = 0; i < 6; ++i) {
        const auto bounds = moduleBounds(i);
        g.setColour(card);
        g.fillRoundedRectangle(bounds.toFloat(), 9.0f);
        g.setColour(accent[size_t(i)]);
        g.drawRoundedRectangle(bounds.toFloat().reduced(1.2f), 9.0f, 2.0f);
        g.setColour(accent[size_t(i)].withAlpha(0.14f));
        g.fillRoundedRectangle(juce::Rectangle<float>(
            float(bounds.getX())+2.0f, float(bounds.getY())+2.0f,
            float(bounds.getWidth())-4.0f, 38.0f), 7.0f);
        g.setColour(pale);
        font(g, 16.0f, true);
        g.drawText(names[size_t(i)], bounds.getX()+12, bounds.getY()+8,
                   148, 24, juce::Justification::centredLeft);

        const auto display = juce::Rectangle<float>(
            float(bounds.getX()+10),float(bounds.getY()+47),188.0f,82.0f);
        drawModulePlot(g, i, display);

        g.setColour(pale);
        font(g, 11.0f, true);
        g.drawText("AMOUNT",bounds.getX()+55,bounds.getY()+130,100,17,
                   juce::Justification::centred);
        g.setColour(muted);
        g.drawText(characterLabels[size_t(i)],bounds.getX()+5,bounds.getY()+262,94,18,
                   juce::Justification::centred);
        g.drawText("FLUX",bounds.getX()+108,bounds.getY()+262,94,18,
                   juce::Justification::centred);
        if (i != 0 && i != 2 && i != 3) {
            g.setColour(juce::Colour(0xff79719d));
            font(g, 9.5f, true);
            g.drawText(subtitles[size_t(i)],bounds.getX()+12,bounds.getY()+358,
                       186,13,juce::Justification::centred);
        }
        if (i == 3) {
            g.setColour(muted);
            font(g, 10, true);
            g.drawText("SMOOTH", bounds.getX()+11,bounds.getBottom()-31,68,16,
                       juce::Justification::centredLeft);
        }
    }

    // Bottom bands: echo + surgical master filters.
    g.setColour(panel);
    g.fillRoundedRectangle(juce::Rectangle<float>(23, 571, 639, 204), 10.0f);
    g.fillRoundedRectangle(juce::Rectangle<float>(674, 571, 664, 204), 10.0f);
    g.setColour(violet);
    g.drawRoundedRectangle(juce::Rectangle<float>(23, 571, 639, 204), 10.0f, 2.0f);
    g.setColour(lime);
    g.drawRoundedRectangle(juce::Rectangle<float>(674, 571, 664, 204), 10.0f, 2.0f);
    g.setColour(pale);
    font(g, 17.0f, true);
    g.drawText("ECHO", 41, 583, 158, 27, juce::Justification::centredLeft);
    g.drawText("MASTER TONE", 694, 583, 270, 27,
               juce::Justification::centredLeft);
    g.setColour(muted);
    font(g, 11, true);
    g.drawText("SYNCED TO FL STUDIO BPM", 219, 591, 269, 18,
               juce::Justification::centredLeft);
    constexpr std::array<const char*,5> bottom {
        "DRY / WET", "FEEDBACK", "STEREO WIDTH", "LOW CUT", "HIGH CUT"
    };
    const std::array<int,5> xPositions {65,486,708,921,1138};
    for (size_t i = 0; i < bottom.size(); ++i)
        g.drawText(bottom[i], xPositions[i], 740, 122, 16,
                   juce::Justification::centred);

    g.setColour(pale);
    font(g, 13, true);
    g.drawText("TIME", 239, 637, 200, 23, juce::Justification::centred);
    const auto idxValue = processor.parameters.getRawParameterValue("echoDivision");
    const int index = idxValue ? int(idxValue->load()) : 3;
    const float bpm = processor.getHostBpm();
    const float ms = dc23::syncedEchoMilliseconds(index, bpm);
    font(g, 12, true);
    g.setColour(lime);
    g.drawText(juce::String(juce::roundToInt(ms)) + " ms @ " +
               juce::String(juce::roundToInt(bpm)) + " BPM",
               231, 720, 200, 25, juce::Justification::centred);
    g.setColour(muted);
    font(g, 9.5f);
    g.drawText("0% DRY    /    50% EVEN    /    100% WET", 43, 761, 362, 14,
               juce::Justification::centredLeft);
    g.drawText("DIOALEX MUSIC TOOLS  •  ORIGINAL DSP  •  VST3", 958, 759, 363, 15,
               juce::Justification::centredRight);
}

void DC23AudioProcessorEditor::resized() {
    auto place = [this](juce::Component& item, int x, int y, int w, int h) {
        item.setBounds(scaledBounds(x, y, w, h));
    };
    const std::array<juce::Slider*,3> top {{&magnitude, &mix, &output}};
    for (int i = 0; i < 3; ++i)
        place(*top[size_t(i)], 360+i*139, 26, 123, 98);
    place(factoryPresets, 820, 54, 275, 33);
    place(saveButton, 1108, 54, 92, 33);
    place(loadButton, 1207, 54, 117, 33);
    place(settingsGear, 1282, 22, 40, 34);
    for (int i = 0; i < 6; ++i) {
        const auto a = moduleBounds(i);
        auto& m = modules[size_t(i)];
        place(m.amount, a.getX()+40, a.getY()+147, 129, 114);
        place(m.character, a.getX()+7, a.getY()+282, 94, 75);
        place(m.flux, a.getX()+107, a.getY()+282, 94, 75);
        place(m.enabled, a.getRight()-57, a.getY()+5, 51, 28);
    }
    place(noiseStyle, moduleBounds(0).getX()+12,
          moduleBounds(0).getBottom()-32, 184, 22);
    place(distortionStyle, moduleBounds(2).getX()+12,
          moduleBounds(2).getBottom()-32, 184, 22);
    place(digitalSmooth, moduleBounds(3).getX()+78,
          moduleBounds(3).getBottom()-31, 122, 20);
    place(echoEnabled, 576, 584, 62, 27);
    place(echoMix, 59, 625, 134, 110);
    place(echoDivision, 246, 675, 188, 33);
    place(echoFeedback, 483, 625, 134, 110);
    place(width, 700, 625, 132, 110);
    place(lowCut, 915, 625, 132, 110);
    place(highCut, 1132, 625, 132, 110);
    place(lowEnabled, 1009, 583, 60, 26);
    place(highEnabled, 1225, 583, 60, 26);
    place(settingsPanel, 969, 148, 345, 180);
    settingsPanel.toFront(false);
}

void DC23AudioProcessorEditor::openPreset(bool saving) {
    chooser = std::make_unique<juce::FileChooser>(
        saving ? "Save DC-23 preset" : "Load DC-23 preset",
        juce::File(), "*.dc23preset");
    const int flags = saving
        ? juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
        : juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    juce::Component::SafePointer<DC23AudioProcessorEditor> safe(this);
    chooser->launchAsync(flags, [safe, saving](const juce::FileChooser& browser) {
        if (safe == nullptr) return;
        auto file = browser.getResult();
        if (file == juce::File()) return;
        if (saving) {
            file = file.withFileExtension(".dc23preset");
            if (auto xml = safe->processor.parameters.copyState().createXml())
                xml->writeTo(file);
        } else if (auto xml = juce::XmlDocument::parse(file)) {
            auto tree = juce::ValueTree::fromXml(*xml);
            if (tree.isValid() && tree.hasType(safe->processor.parameters.state.getType()))
                safe->processor.parameters.replaceState(tree);
        }
    });
}
