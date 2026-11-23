#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace {
const juce::Colour lime(0xffb9ef49);
const juce::Colour violet(0xffa88dea);
const juce::Colour ink(0xff101314);
const juce::Colour surface(0xff282b2d);
const juce::Colour pale(0xffe7e9e8);
const juce::Colour muted(0xff9da3a2);
constexpr std::array<const char*, 6> names {
    "NOISE", "WOBBLE", "DISTORT", "DIGITAL", "SPACE", "MAGNETIC"
};
constexpr std::array<const char*, 6> subtitle {
    "TAPE / DUST", "WOW / FLUTTER", "SATURATION", "BIT / RATE",
    "REFLECTIONS", "DROP / WEAR"
};
constexpr std::array<const char*, 6> amounts {
    "noise", "wobble", "distort", "digital", "space", "magnetic"
};
constexpr std::array<const char*, 6> characters {
    "noiseColor", "wobbleRate", "distortionTone", "digitalRate",
    "spaceSize", "magneticSpeed"
};
constexpr std::array<const char*, 6> characterLabels {
    "COLOR", "RATE", "TONE", "CRUSH", "SIZE", "SPEED"
};
constexpr std::array<const char*, 6> enables {
    "noiseOn", "wobbleOn", "distortOn", "digitalOn", "spaceOn", "magneticOn"
};

void setFont(juce::Graphics& g, float size, bool bold = false) {
    g.setFont(juce::Font(juce::FontOptions(size, bold
        ? juce::Font::bold : juce::Font::plain)));
}
}

DC23LookAndFeel::DC23LookAndFeel() {
    setColour(juce::Slider::textBoxTextColourId, pale);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff1a1d1e));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId, surface);
    setColour(juce::ComboBox::textColourId, pale);
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff495052));
    setColour(juce::PopupMenu::backgroundColourId, surface);
    setColour(juce::PopupMenu::textColourId, pale);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, lime);
    setColour(juce::PopupMenu::highlightedTextColourId, ink);
    setColour(juce::TextButton::buttonColourId, surface);
    setColour(juce::TextButton::textColourOffId, pale);
}

void DC23LookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                                      float position, float start, float end,
                                      juce::Slider&) {
    const float size = static_cast<float>(std::min(w, h)) * 0.78f;
    const float cx = x + w * 0.5f, cy = y + h * 0.47f;
    const float radius = size * 0.45f;
    g.setColour(juce::Colour(0xff16191a));
    g.fillEllipse(cx - radius, cy - radius, radius * 2, radius * 2);
    juce::Path base, active;
    base.addCentredArc(cx, cy, radius + 6.0f, radius + 6.0f, 0,
                       start, end, true);
    active.addCentredArc(cx, cy, radius + 6.0f, radius + 6.0f, 0,
                         start, start + position * (end - start), true);
    g.setColour(juce::Colour(0xff454b4b));
    g.strokePath(base, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));
    g.setColour(lime);
    g.strokePath(active, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    const float angle = start + position * (end - start);
    g.setColour(violet);
    g.drawLine(cx + std::sin(angle) * radius * 0.25f,
               cy - std::cos(angle) * radius * 0.25f,
               cx + std::sin(angle) * radius * 0.74f,
               cy - std::cos(angle) * radius * 0.74f, 3.0f);
    g.setColour(juce::Colour(0xff666f6b));
    g.drawEllipse(cx - radius, cy - radius, radius * 2, radius * 2, 1.0f);
}

void DC23LookAndFeel::drawToggleButton(juce::Graphics& g,
                                       juce::ToggleButton& button, bool, bool) {
    auto r = button.getLocalBounds().toFloat().reduced(2.0f);
    g.setColour(button.getToggleState() ? lime : juce::Colour(0xff505756));
    g.fillRoundedRectangle(r, 7.0f);
    g.setColour(ink);
    g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    g.drawText(button.getToggleState() ? "ON" : "OFF",
               button.getLocalBounds(), juce::Justification::centred, false);
}

juce::Rectangle<int> DC23AudioProcessorEditor::moduleBounds(int index) {
    return { 24 + (index % 3) * 341, 165 + (index / 3) * 190, 328, 180 };
}

DC23AudioProcessorEditor::DC23AudioProcessorEditor(DC23AudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p) {
    setLookAndFeel(&look);
    for (size_t i = 0; i < modules.size(); ++i) {
        auto& m = modules[i];
        initialiseSlider(m.amount, amounts[i], m.amountLink);
        initialiseSlider(m.character, characters[i], m.characterLink);
        m.enabled.setClickingTogglesState(true);
        addAndMakeVisible(m.enabled);
        m.enabledLink = std::make_unique<ButtonAttachment>(processor.parameters,
                                                            enables[i], m.enabled);
    }

    initialiseSlider(magnitude, "magnitude", masterLinks[0]);
    initialiseSlider(mix, "mix", masterLinks[1]);
    initialiseSlider(output, "outputDb", masterLinks[2]);
    initialiseSlider(lowCut, "lowCut", masterLinks[3]);
    initialiseSlider(highCut, "highCut", masterLinks[4]);

    factoryPresets.addItem("INIT", 1);
    factoryPresets.addItem("VHS GLOW", 2);
    factoryPresets.addItem("DUSTY KEYS", 3);
    factoryPresets.addItem("DRUM WEAR", 4);
    factoryPresets.addItem("TAPE DRIFT", 5);
    factoryPresets.addItem("BROKEN RADIO", 6);
    factoryPresets.setSelectedId(1, juce::dontSendNotification);
    factoryPresets.onChange = [this] {
        if (factoryPresets.getSelectedId() > 0)
            processor.applyFactoryPreset(factoryPresets.getSelectedId() - 1);
    };
    addAndMakeVisible(factoryPresets);

    saveButton.onClick = [this] { openPreset(true); };
    loadButton.onClick = [this] { openPreset(false); };
    addAndMakeVisible(saveButton);
    addAndMakeVisible(loadButton);
    setSize(1060, 660);
}

DC23AudioProcessorEditor::~DC23AudioProcessorEditor() {
    chooser.reset();
    setLookAndFeel(nullptr);
}

void DC23AudioProcessorEditor::initialiseSlider(
        juce::Slider& slider, const juce::String& id,
        std::unique_ptr<SliderAttachment>& attachment) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 19);
    slider.setDoubleClickReturnValue(true, 0.0);
    slider.setMouseDragSensitivity(180);
    slider.setPopupDisplayEnabled(true, false, this);
    addAndMakeVisible(slider);
    attachment = std::make_unique<SliderAttachment>(processor.parameters, id, slider);
}

void DC23AudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(ink);
    g.setColour(lime);
    g.fillRoundedRectangle(juce::Rectangle<float>(19, 16, 1022, 136), 12.0f);
    g.setColour(ink);
    setFont(g, 58.0f, true);
    g.drawText("DC-23", 39, 24, 296, 76, juce::Justification::centredLeft);
    setFont(g, 13.0f, true);
    g.drawText("VINTAGE COLOR ENGINE", 46, 99, 260, 28,
               juce::Justification::centredLeft);

    g.setColour(ink);
    setFont(g, 12.0f, true);
    for (int i = 0; i < 3; ++i) {
        constexpr const char* labels[3] = { "MAGNITUDE", "DRY / WET", "OUTPUT" };
        g.drawText(labels[i], 379 + i * 109, 123, 106, 18, juce::Justification::centred);
    }
    g.drawText("PRESETS", 752, 30, 214, 20, juce::Justification::centredLeft);

    for (int i = 0; i < static_cast<int>(modules.size()); ++i) {
        const auto area = moduleBounds(i);
        g.setColour(surface);
        g.fillRoundedRectangle(area.toFloat(), 11.0f);
        g.setColour(lime);
        g.fillRoundedRectangle(juce::Rectangle<float>(static_cast<float>(area.getX()),
                              static_cast<float>(area.getY()), 328.0f, 34.0f), 8.0f);
        g.setColour(ink);
        setFont(g, 18.0f, true);
        g.drawText(names[static_cast<size_t>(i)], area.getX() + 13, area.getY() + 3,
                   153, 26, juce::Justification::centredLeft);
        setFont(g, 10.0f, true);
        g.drawText(subtitle[static_cast<size_t>(i)], area.getX() + 153,
                   area.getY() + 4, 97, 25, juce::Justification::centredRight);
        g.setColour(muted);
        setFont(g, 11.0f, true);
        g.drawText("AMOUNT", area.getX() + 21, area.getBottom() - 31, 101, 18,
                   juce::Justification::centred);
        g.drawText(characterLabels[static_cast<size_t>(i)], area.getX() + 161,
                   area.getBottom() - 31, 101, 18, juce::Justification::centred);
        g.setColour(juce::Colour(0xff515758));
        g.drawLine(static_cast<float>(area.getX()+145),
                   static_cast<float>(area.getY()+46),
                   static_cast<float>(area.getX()+145),
                   static_cast<float>(area.getBottom()-21), 1.0f);
    }

    g.setColour(surface);
    g.fillRoundedRectangle(juce::Rectangle<float>(24, 545, 1010, 99), 10.0f);
    g.setColour(lime);
    setFont(g, 15, true);
    g.drawText("DC-23 / MASTER TONE", 44, 557, 320, 29,
               juce::Justification::centredLeft);
    g.setColour(muted);
    setFont(g, 13);
    g.drawText("Independent color modules / handcrafted DSP / native VST3",
               44, 592, 530, 24, juce::Justification::centredLeft);
    g.setColour(pale);
    setFont(g, 12, true);
    g.drawText("LOW CUT", 739, 620, 120, 20, juce::Justification::centred);
    g.drawText("HIGH CUT", 876, 620, 120, 20, juce::Justification::centred);
}

void DC23AudioProcessorEditor::resized() {
    auto controls = std::array<juce::Slider*, 3> { &magnitude, &mix, &output };
    for (size_t i = 0; i < controls.size(); ++i)
        controls[i]->setBounds(378 + static_cast<int>(i) * 109, 22, 106, 104);

    factoryPresets.setBounds(752, 53, 276, 28);
    saveButton.setBounds(752, 96, 130, 29);
    loadButton.setBounds(893, 96, 135, 29);

    for (int i = 0; i < static_cast<int>(modules.size()); ++i) {
        const auto area = moduleBounds(i);
        auto& m = modules[static_cast<size_t>(i)];
        m.amount.setBounds(area.getX()+19, area.getY()+41, 112, 116);
        m.character.setBounds(area.getX()+158, area.getY()+41, 112, 116);
        m.enabled.setBounds(area.getRight()-66, area.getY()+5, 54, 24);
    }
    lowCut.setBounds(747, 548, 106, 76);
    highCut.setBounds(885, 548, 106, 76);
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
            auto xml = safe->processor.parameters.copyState().createXml();
            if (xml != nullptr) xml->writeTo(file);
        } else {
            auto xml = juce::XmlDocument::parse(file);
            if (xml != nullptr) {
                auto tree = juce::ValueTree::fromXml(*xml);
                if (tree.isValid() && tree.hasType(safe->processor.parameters.state.getType()))
                    safe->processor.parameters.replaceState(tree);
            }
        }
    });
}
