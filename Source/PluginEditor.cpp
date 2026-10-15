#include "PluginEditor.h"

NocturneDarkSynthAudioProcessorEditor::NocturneDarkSynthAudioProcessorEditor(NocturneDarkSynthAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p),
      keyboard(p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    addKnob("CUTOFF", "cutoff");   addKnob("RESONANCE", "resonance");
    addKnob("DRIVE", "drive");     addKnob("PHASER", "phaser");
    addKnob("LFO RATE", "rate");   addKnob("MOD DEPTH", "depth");
    addKnob("ATTACK", "attack");   addKnob("RELEASE", "release");
    addKnob("DETUNE", "detune");   addKnob("SUB", "sub");

    // Waveform selector
    waveLabel.setText("WAVEFORM", juce::dontSendNotification);
    waveLabel.setColour(juce::Label::textColourId, juce::Colour(0xffbdb4d9));
    addAndMakeVisible(waveLabel);
    waveBox.addItemList(juce::StringArray { "Saw", "Square", "Triangle" }, 1);
    addAndMakeVisible(waveBox);
    waveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.parameters, "wave", waveBox);

    // Sound banks
    for (auto& bank : Presets::banks())
        bankBox.addItem(bank.name, bankBox.getNumItems() + 1);
    addAndMakeVisible(bankBox);
    addAndMakeVisible(patchBox);
    addAndMakeVisible(prevButton);
    addAndMakeVisible(nextButton);

    bankBox.onChange = [this] {
        const int bank = bankBox.getSelectedItemIndex();
        if (bank < 0) return;
        fillPatchBox(bank);
        processor.applyPatch(Presets::indexOf(bank, 0));
        syncPatchUi();
    };
    patchBox.onChange = [this] {
        const int bank = bankBox.getSelectedItemIndex(), patch = patchBox.getSelectedItemIndex();
        if (bank >= 0 && patch >= 0)
            processor.applyPatch(Presets::indexOf(bank, patch));
    };
    prevButton.onClick = [this] { stepPatch(-1); };
    nextButton.onClick = [this] { stepPatch(+1); };

    keyboard.setKeyWidth(30.0f);
    keyboard.setAvailableRange(24, 108);
    keyboard.setLowestVisibleKey(36);
    keyboard.setWantsKeyboardFocus(true);
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colour(0xffa58bff).withAlpha(0.7f));
    keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, juce::Colour(0xffa58bff).withAlpha(0.25f));
    addAndMakeVisible(keyboard);

    syncPatchUi();
    startTimerHz(8);   // follow program changes coming from the host

    // Set the size last so resized() lays out the children that now exist.
    setSize(680, 470);
}

void NocturneDarkSynthAudioProcessorEditor::addKnob(const juce::String& title, const juce::String& id)
{
    auto* s = knobs.add(new juce::Slider());
    s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 18);
    s->setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffa58bff));
    s->setColour(juce::Slider::thumbColourId, juce::Colour(0xfff0eaff));
    s->setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffeeeaf8));
    addAndMakeVisible(s);
    auto* l = labels.add(new juce::Label());
    l->setText(title, juce::dontSendNotification);
    l->setJustificationType(juce::Justification::centred);
    l->setColour(juce::Label::textColourId, juce::Colour(0xffbdb4d9));
    addAndMakeVisible(l);
    attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters, id, *s));
}

void NocturneDarkSynthAudioProcessorEditor::fillPatchBox(int bank)
{
    patchBox.clear(juce::dontSendNotification);
    int id = 1;
    for (auto& patch : Presets::banks()[(size_t) bank].patches)
        patchBox.addItem(patch.name, id++);
}

void NocturneDarkSynthAudioProcessorEditor::syncPatchUi()
{
    int bank = 0, patch = 0;
    Presets::locate(processor.getCurrentProgram(), bank, patch);

    if (bankBox.getSelectedItemIndex() != bank || patchBox.getNumItems() == 0)
    {
        bankBox.setSelectedItemIndex(bank, juce::dontSendNotification);
        fillPatchBox(bank);
    }
    if (patchBox.getSelectedItemIndex() != patch)
        patchBox.setSelectedItemIndex(patch, juce::dontSendNotification);
}

void NocturneDarkSynthAudioProcessorEditor::stepPatch(int delta)
{
    const int total = Presets::totalPatches();
    const int next = (processor.getCurrentProgram() + delta + total) % total;
    processor.applyPatch(next);
    syncPatchUi();
}

void NocturneDarkSynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff090912));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff1c1630), 0, 0,
                                           juce::Colour(0xff090912), (float) getWidth(), (float) getHeight(), false));
    g.fillRect(getLocalBounds());
    g.setColour(juce::Colour(0xffa58bff));
    g.setFont(juce::Font(25.0f, juce::Font::bold));
    g.drawText("NOCTURNE", 24, 14, 260, 34, juce::Justification::left);
    g.setColour(juce::Colour(0xff8f86a8));
    g.setFont(juce::Font(10.5f));
    g.drawText("DARK SYNTH  /  SOUND BANKS  /  MODULATION LAB", 26, 47, 340, 18, juce::Justification::left);
}

void NocturneDarkSynthAudioProcessorEditor::resized()
{
    prevButton.setBounds(372, 18, 28, 26);
    bankBox.setBounds(404, 18, 108, 26);
    patchBox.setBounds(516, 18, 120, 26);
    nextButton.setBounds(640, 18, 28, 26);

    const int startY = 76, cellH = 105;
    for (int i = 0; i < knobs.size(); ++i) {
        int col = i % 5, row = i / 5;
        knobs[i]->setBounds(18 + col * 132, startY + row * cellH, 92, 78);
        labels[i]->setBounds(7 + col * 132, startY + row * cellH - 4, 112, 20);
    }

    waveLabel.setBounds(24, 272, 140, 18);
    waveBox.setBounds(24, 292, 140, 30);
    keyboard.setBounds(20, 346, getWidth() - 40, 100);
}
