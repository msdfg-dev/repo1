#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class NocturneDarkSynthAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit NocturneDarkSynthAudioProcessorEditor(NocturneDarkSynthAudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    NocturneDarkSynthAudioProcessor& processor;
    juce::OwnedArray<juce::Slider> knobs;
    juce::OwnedArray<juce::Label> labels;
    juce::TextButton loadButton { "LOAD SAMPLE" };
    juce::Label sampleLabel;
    std::unique_ptr<juce::FileChooser> chooser;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
    void addKnob(const juce::String& title, const juce::String& id);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NocturneDarkSynthAudioProcessorEditor)
};
