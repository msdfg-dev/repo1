#include "PluginEditor.h"

NocturneDarkSynthAudioProcessorEditor::NocturneDarkSynthAudioProcessorEditor(NocturneDarkSynthAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(680, 430);
    addKnob("CUTOFF", "cutoff"); addKnob("RESONANCE", "resonance");
    addKnob("DRIVE", "drive"); addKnob("PHASER", "phaser");
    addKnob("LFO RATE", "rate"); addKnob("MOD DEPTH", "depth");
    addKnob("ATTACK", "attack"); addKnob("RELEASE", "release");
    addKnob("SAMPLER MIX", "samplemix");
    addAndMakeVisible(loadButton);
    loadButton.onClick = [this] {
        chooser = std::make_unique<juce::FileChooser>("Choose a sample", juce::File{}, "*.wav;*.aif;*.aiff;*.flac;*.ogg");
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc) {
                if (fc.getResult().existsAsFile() && processor.loadSample(fc.getResult()))
                    sampleLabel.setText("SAMPLE: " + processor.getSampleName(), juce::dontSendNotification);
            });
    };
    addAndMakeVisible(sampleLabel);
    sampleLabel.setText("SAMPLE: " + processor.getSampleName(), juce::dontSendNotification);
    sampleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffc9c5d6));
    sampleLabel.setJustificationType(juce::Justification::centredLeft);
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
void NocturneDarkSynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff090912));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff1c1630), 0, 0, juce::Colour(0xff090912), 680, 430, false));
    g.fillRect(getLocalBounds());
    g.setColour(juce::Colour(0xffa58bff));
    g.setFont(juce::Font(25.0f, juce::Font::bold));
    g.drawText("NOCTURNE", 24, 14, 260, 34, juce::Justification::left);
    g.setColour(juce::Colour(0xff8f86a8));
    g.setFont(juce::Font(10.5f));
    g.drawText("DARK SYNTH  /  MODULATION LAB  /  SAMPLER", 26, 47, 420, 18, juce::Justification::left);
}
void NocturneDarkSynthAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    const int startY = 76, cellW = 128, cellH = 105;
    for (int i = 0; i < knobs.size(); ++i) {
        int col = i % 5, row = i / 5;
        knobs[i]->setBounds(18 + col * 132, startY + row * cellH, 92, 78);
        labels[i]->setBounds(7 + col * 132, startY + row * cellH - 4, 112, 20);
    }
    loadButton.setBounds(24, 345, 145, 34);
    loadButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff34264f));
    loadButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffeeeaf8));
    sampleLabel.setBounds(182, 345, 470, 34);
}
