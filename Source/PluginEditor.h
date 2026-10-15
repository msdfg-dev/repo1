#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class NocturneDarkSynthAudioProcessorEditor : public juce::AudioProcessorEditor,
                                              private juce::Timer
{
public:
    explicit NocturneDarkSynthAudioProcessorEditor(NocturneDarkSynthAudioProcessor&);
    ~NocturneDarkSynthAudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override { syncPatchUi(); }
    void addKnob(const juce::String& title, const juce::String& id);
    void fillPatchBox(int bank);
    void syncPatchUi();
    void stepPatch(int delta);

    NocturneDarkSynthAudioProcessor& processor;
    juce::OwnedArray<juce::Slider> knobs;
    juce::OwnedArray<juce::Label> labels;
    juce::ComboBox bankBox, patchBox, waveBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    juce::Label waveLabel;
    juce::MidiKeyboardComponent keyboard;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveAttachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NocturneDarkSynthAudioProcessorEditor)
};
