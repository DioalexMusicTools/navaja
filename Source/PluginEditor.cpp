#include "PluginEditor.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace {
const juce::Colour lime(0xffb9ef49);
const juce::Colour ink(0xff0e1011);
const juce::Colour surface(0xff202426);
const juce::Colour panel(0xff2a2e30);
const juce::Colour pale(0xffecf1ec);
const juce::Colour muted(0xffa5acab);
const juce::Colour violet(0xffbd9cfa);
constexpr std::array<const char*, 6> names {
    "NOISE", "WOBBLE", "DISTORT", "DIGITAL", "SPACE", "MAGNETIC"
};
constexpr std::array<const char*, 6> subtitles {
    "TAPE / DUST", "TAPE / MOTION", "COLOR / DRIVE",
    "BIT / RATE", "ROOM / DEPTH", "WEAR / LOSS"
};
constexpr std::array<const char*, 6> amounts {
    "noise", "wobble", "distort", "digital", "space", "magnetic"
};
constexpr std::array<const char*, 6> characters {
    "noiseColor", "wobbleRate", "distortionTone", "digitalRate",
    "spaceSize", "magneticSpeed"
};
constexpr std::array<const char*, 6> labels {
    "COLOR", "RATE", "TONE", "CRUSH", "DECAY", "SPEED"
};
constexpr std::array<const char*, 6> enables {
    "noiseOn", "wobbleOn", "distortOn", "digitalOn", "spaceOn", "magneticOn"
};
constexpr std::array<const char*, 6> fluxIds {
    "noiseFlux", "wobbleFlux", "distortFlux",
    "digitalFlux", "spaceFlux", "magneticFlux"
};
const std::array<juce::Colour, 6> accents {
    juce::Colour(0xffb9ef49), juce::Colour(0xffe5ba66),
    juce::Colour(0xffbb95fa), juce::Colour(0xff85d4e5),
    juce::Colour(0xff69cbae), juce::Colour(0xffd5a6de)
};

void font(juce::Graphics& g, float size, bool bold = false) {
    g.setFont(juce::Font(juce::FontOptions(size,
             bold ? juce::Font::bold : juce::Font::plain)));
}
}

DC23LookAndFeel::DC23LookAndFeel() {
    setColour(juce::Slider::textBoxTextColourId, pale);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff151819));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId, surface);
    setColour(juce::ComboBox::textColourId, pale);
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff5c6465));
    setColour(juce::PopupMenu::backgroundColourId, panel);
    setColour(juce::PopupMenu::textColourId, pale);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, lime);
    setColour(juce::PopupMenu::highlightedTextColourId, ink);
    setColour(juce::TextButton::buttonColourId, panel);
    setColour(juce::TextButton::textColourOffId, pale);
}

void DC23LookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                                      float pos, float start, float end, juce::Slider& slider) {
    const float size = static_cast<float>(std::min(w, h)) * 0.75f;
    const float cx = x + 0.5f * w;
    const float cy = y + 0.45f * h;
    const float radius = size * 0.43f;
    const auto active = slider.findColour(juce::Slider::rotarySliderFillColourId, true)
                       .withMultipliedBrightness(1.3f);

    g.setColour(juce::Colour(0xff111414));
    g.fillEllipse(cx-radius, cy-radius, radius*2.0f, radius*2.0f);
    juce::Path path, progress;
    path.addCentredArc(cx, cy, radius+5.0f, radius+5.0f, 0, start, end, true);
    progress.addCentredArc(cx, cy, radius+5.0f, radius+5.0f, 0,
                           start, start + pos*(end-start), true);
    g.setColour(juce::Colour(0xff495050));
    g.strokePath(path, juce::PathStrokeType(4.6f));
    g.setColour(active);
    g.strokePath(progress, juce::PathStrokeType(4.6f));
    const float angle = start + pos * (end - start);
    g.setColour(violet);
    g.drawLine(cx + std::sin(angle)*radius*0.2f,
               cy - std::cos(angle)*radius*0.2f,
               cx + std::sin(angle)*radius*0.78f,
               cy - std::cos(angle)*radius*0.78f, 3.0f);
    g.setColour(juce::Colour(0xff727978));
    g.drawEllipse(cx-radius, cy-radius, 2.0f*radius, 2.0f*radius, 1.0f);
}

void DC23LookAndFeel::drawToggleButton(juce::Graphics& g,
                                       juce::ToggleButton& b, bool, bool) {
    auto r = b.getLocalBounds().toFloat().reduced(2.0f);
    g.setColour(b.getToggleState() ? lime : juce::Colour(0xff636a67));
    g.fillRoundedRectangle(r, 6.0f);
    g.setColour(ink);
    font(g, 11.5f, true);
    g.drawText(b.getToggleState() ? "ON" : "OFF",
               b.getLocalBounds(), juce::Justification::centred);
}

juce::Rectangle<int> DC23AudioProcessorEditor::moduleBounds(int index) {
    return {24 + index * 199, 163, 191, 338};
}

DC23AudioProcessorEditor::DC23AudioProcessorEditor(DC23AudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p) {
    setLookAndFeel(&look);
    for (size_t i = 0; i < modules.size(); ++i) {
        auto& m = modules[i];
        setupKnob(m.amount, amounts[i], m.amountLink);
        setupKnob(m.character, characters[i], m.characterLink);
        setupKnob(m.flux, fluxIds[i], m.fluxLink);
        m.amount.setColour(juce::Slider::rotarySliderFillColourId, accents[i]);
        m.character.setColour(juce::Slider::rotarySliderFillColourId, accents[i]);
        m.flux.setColour(juce::Slider::rotarySliderFillColourId, violet);
        m.amount.setTooltip(juce::String(names[i]) + " amount");
        m.character.setTooltip(juce::String(labels[i]) + " character");
        m.flux.setTooltip("Organic amount variation (Flux)");
        m.amount.onValueChange = [this] { repaint(); };
        m.enabled.setClickingTogglesState(true);
        addAndMakeVisible(m.enabled);
        m.enabledLink = std::make_unique<ButtonAttachment>(processor.parameters,
                                                            enables[i], m.enabled);
    }

    setupKnob(magnitude, "magnitude", masterLinks[0]);
    setupKnob(mix, "mix", masterLinks[1]);
    setupKnob(output, "outputDb", masterLinks[2]);
    setupKnob(echoMix, "echoMix", masterLinks[3]);
    setupKnob(echoTime, "echoTimeMs", masterLinks[4]);
    setupKnob(echoFeedback, "echoFeedback", masterLinks[5]);
    setupKnob(width, "width", masterLinks[6]);
    setupKnob(lowCut, "lowCut", masterLinks[7]);
    setupKnob(highCut, "highCut", masterLinks[8]);
    setupKnob(digitalSmooth, "digitalSmooth", masterLinks[9]);
    magnitude.setTooltip("Overall amount of all effects including echo");
    mix.setTooltip("Dry / Wet");
    echoTime.setTooltip("Echo time in milliseconds");
    width.setTooltip("0 mono / 1 normal / 2 wide");
    digitalSmooth.setSliderStyle(juce::Slider::LinearHorizontal);
    digitalSmooth.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

    noiseStyle.addItemList(juce::StringArray{"TAPE", "VINYL", "HUM"}, 1);
    distortionStyle.addItemList(juce::StringArray{"TAPE", "VALVE", "FUZZ"}, 1);
    noiseStyleLink = std::make_unique<ComboAttachment>(processor.parameters, "noiseStyle", noiseStyle);
    distortionStyleLink = std::make_unique<ComboAttachment>(processor.parameters,
                                                            "distortionStyle", distortionStyle);
    addAndMakeVisible(noiseStyle);
    addAndMakeVisible(distortionStyle);

    factoryPresets.setTextWhenNothingSelected("SELECT PRESET");
    factoryPresets.addItemList(juce::StringArray{
        "INIT", "VHS GLOW", "DUSTY KEYS", "DRUM WEAR", "TAPE DRIFT", "BROKEN RADIO"
    }, 1);
    factoryPresets.onChange = [this] {
        const int id = factoryPresets.getSelectedId();
        if (id >= 1 && id <= 6) processor.applyFactoryPreset(id - 1);
    };
    addAndMakeVisible(factoryPresets);
    saveButton.onClick = [this] { openPreset(true); };
    loadButton.onClick = [this] { openPreset(false); };
    addAndMakeVisible(saveButton);
    addAndMakeVisible(loadButton);

    setSize(1240, 720);
}

DC23AudioProcessorEditor::~DC23AudioProcessorEditor() {
    chooser.reset();
    setLookAndFeel(nullptr);
}

void DC23AudioProcessorEditor::setupKnob(
        juce::Slider& slider, const juce::String& id,
        std::unique_ptr<SliderAttachment>& attachment) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 18);
    slider.setMouseDragSensitivity(170);
    slider.setPopupDisplayEnabled(true, false, this);
    slider.setColour(juce::Slider::rotarySliderFillColourId, lime);
    if (auto* param = processor.parameters.getParameter(id)) {
        slider.setDoubleClickReturnValue(true,
              param->convertFrom0to1(param->getDefaultValue()));
    }
    addAndMakeVisible(slider);
    attachment = std::make_unique<SliderAttachment>(processor.parameters, id, slider);
}

void DC23AudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(ink);
    g.setColour(juce::Colour(0xff141718));
    g.fillRoundedRectangle(juce::Rectangle<float>(13, 11, 1214, 698), 13.0f);

    g.setGradientFill(juce::ColourGradient(lime, 20.0f, 18.0f,
                      juce::Colour(0xff8bc82d), 1208.0f, 127.0f, false));
    g.fillRoundedRectangle(juce::Rectangle<float>(20, 17, 1200, 130), 11.0f);

    g.setColour(ink);
    font(g, 67, true);
    g.drawText("DC-23", 37, 25, 285, 86, juce::Justification::centredLeft);
    font(g, 14, true);
    g.drawText("VINTAGE COLOR LAB", 42, 111, 277, 24, juce::Justification::centredLeft);
    font(g, 12, true);
    constexpr std::array<const char*, 3> master {
        "MAGNITUDE", "DRY / WET", "OUTPUT"
    };
    for (int i = 0; i < 3; ++i)
        g.drawText(master[static_cast<size_t>(i)], 346+i*124, 120, 114, 17,
                   juce::Justification::centred);

    g.drawText("PRESET", 762, 30, 185, 18, juce::Justification::centredLeft);
    for (size_t i = 0; i < modules.size(); ++i) {
        const auto a = moduleBounds(static_cast<int>(i));
        const auto accent = accents[i];
        g.setColour(surface);
        g.fillRoundedRectangle(a.toFloat(), 10.0f);
        g.setColour(juce::Colour(0xff454c4d));
        g.drawRoundedRectangle(a.toFloat().reduced(0.5f), 10.0f, 1.0f);
        g.setColour(accent);
        g.fillRoundedRectangle(juce::Rectangle<float>(float(a.getX()), float(a.getY()),
                               float(a.getWidth()), 38.0f), 8.0f);
        g.setColour(ink);
        font(g, 17, true);
        g.drawText(names[i], a.getX()+10, a.getY()+6, 120, 27,
                   juce::Justification::centredLeft);

        // Original visual indicator: each card has its own sound-signature trace.
        const auto display = juce::Rectangle<float>(
            float(a.getX()+12), float(a.getY()+45), 167.0f, 53.0f);
        g.setColour(juce::Colour(0xff111415));
        g.fillRoundedRectangle(display, 5.0f);
        juce::Path trace;
        const float depth = float(modules[i].amount.getValue());
        for (int px = 0; px <= 160; px += 2) {
            const float t = float(px) * 0.08f;
            float s = std::sin(t * (0.6f+0.13f*float(i))) * std::cos(t*0.37f+float(i));
            if (i == 0) s = std::sin(t*8.3f)*0.4f + std::sin(t*1.1f)*0.18f;
            if (i == 3) s = std::round(s*3.0f)*0.3333f;
            if (i == 5) s *= 0.3f + 0.7f*std::abs(std::sin(t*0.33f));
            const float x = display.getX()+4.0f+float(px);
            const float y = display.getCentreY()-s*(4.0f+13.0f*depth);
            if (px == 0) trace.startNewSubPath(x, y);
            else trace.lineTo(x, y);
        }
        g.setColour(accent.withAlpha(0.8f));
        g.strokePath(trace, juce::PathStrokeType(1.9f));

        g.setColour(muted);
        font(g, 11.0f, true);
        g.drawText("AMOUNT", a.getX()+43, a.getY()+105, 100, 16, juce::Justification::centred);
        g.drawText(labels[i], a.getX()+10, a.getY()+213, 75, 16, juce::Justification::centred);
        g.drawText("FLUX", a.getX()+104, a.getY()+213, 75, 16,
                   juce::Justification::centred);

        if (i == 3) {
            g.setColour(pale);
            font(g, 10.0f, true);
            g.drawText("SMOOTH", a.getX()+10, a.getY()+316, 62, 18,
                       juce::Justification::centredLeft);
        } else if (i != 0 && i != 2) {
            g.setColour(juce::Colour(0xff7f8989));
            font(g, 9.5f, true);
            g.drawText(subtitles[i], a.getX()+7, a.getY()+317, a.getWidth()-14, 17,
                       juce::Justification::centred);
        }
    }

    g.setColour(panel);
    g.fillRoundedRectangle(juce::Rectangle<float>(24, 514, 1192, 190), 11.0f);
    g.setColour(juce::Colour(0xff4d5555));
    g.drawLine(619.0f, 525.0f, 619.0f, 689.0f, 1.0f);

    g.setColour(lime);
    font(g, 17, true);
    g.drawText("ECHO  /  TAPE REPEATS", 42, 521, 530, 25,
               juce::Justification::centredLeft);
    g.drawText("MASTER  /  TONE SHAPER", 647, 521, 522, 25,
               juce::Justification::centredLeft);

    g.setColour(muted);
    font(g, 11.5f, true);
    constexpr std::array<const char*, 6> bottom {
        "MIX", "TIME", "FEEDBACK", "STEREO WIDTH", "LOW CUT", "HIGH CUT"
    };
    for (int i = 0; i < 6; ++i) {
        const int base = i < 3 ? 84 + i*166 : 692 + (i-3)*166;
        g.drawText(bottom[static_cast<size_t>(i)], base, 664, 112, 19,
                   juce::Justification::centred);
    }
    g.setColour(pale.withAlpha(0.60f));
    font(g, 10.5f, true);
    g.drawText("DC-23   /   ORIGINAL DSP   /   0.2.0",
               42, 688, 426, 17, juce::Justification::centredLeft);
    g.drawText("FL STUDIO  •  macOS VST3  •  DIOALEX MUSIC TOOLS",
               794, 688, 397, 17, juce::Justification::centredRight);
}

void DC23AudioProcessorEditor::resized() {
    std::array<juce::Slider*, 3> top {{&magnitude, &mix, &output}};
    for (int i = 0; i < 3; ++i)
        top[static_cast<size_t>(i)]->setBounds(344+i*124, 25, 116, 97);

    factoryPresets.setBounds(763, 53, 258, 33);
    saveButton.setBounds(1035, 53, 78, 33);
    loadButton.setBounds(1120, 53, 83, 33);
    for (int i = 0; i < 6; ++i) {
        const auto a = moduleBounds(i);
        auto& m = modules[static_cast<size_t>(i)];
        m.amount.setBounds(a.getX()+41, a.getY()+120, 110, 96);
        m.character.setBounds(a.getX()+9, a.getY()+232, 81, 79);
        m.flux.setBounds(a.getX()+102, a.getY()+232, 81, 79);
        m.enabled.setBounds(a.getRight()-56, a.getY()+5, 49, 27);
    }
    noiseStyle.setBounds(moduleBounds(0).getX()+8, 479, 175, 19);
    distortionStyle.setBounds(moduleBounds(2).getX()+8, 479, 175, 19);
    digitalSmooth.setBounds(moduleBounds(3).getX()+72, 480, 102, 18);

    std::array<juce::Slider*, 6> bottom {
        &echoMix, &echoTime, &echoFeedback, &width, &lowCut, &highCut
    };
    for (int i = 0; i < 6; ++i) {
        const int x = i < 3 ? 84 + i*166 : 692 + (i-3)*166;
        bottom[static_cast<size_t>(i)]->setBounds(x, 548, 112, 112);
    }
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
        } else {
            if (auto xml = juce::XmlDocument::parse(file)) {
                auto tree = juce::ValueTree::fromXml(*xml);
                if (tree.isValid() && tree.hasType(safe->processor.parameters.state.getType()))
                    safe->processor.parameters.replaceState(tree);
            }
        }
    });
}
