#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SynthLookAndFeel.h"

// One oscillator "window": a live waveform display plus Level / Octave / Fine /
// Shape knobs bound to that oscillator's parameters.
class OscWindow : public juce::Component, private juce::Timer
{
public:
    enum class Type { Sine, Triangle, Saw };

    OscWindow(juce::AudioProcessorValueTreeState& apvts, Type type, const juce::String& prefix,
              const juce::String& title, const juce::String& shapeName);

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void timerCallback() override;
    void setupKnob(Knob& k, juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
                   const juce::String& text);

    Type oscType;
    juce::String titleText;
    std::atomic<float>* shapeParam = nullptr;
    std::atomic<float>* levelParam = nullptr;
    std::array<Knob, 4> knobs;   // level, octave, fine, shape
    juce::Rectangle<int> displayArea;
    float lastShape = -1.0f, lastLevel = -1.0f;
};

class NocturneDarkSynthAudioProcessorEditor : public juce::AudioProcessorEditor,
                                              private juce::Timer
{
public:
    explicit NocturneDarkSynthAudioProcessorEditor(NocturneDarkSynthAudioProcessor&);
    ~NocturneDarkSynthAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override { syncPatchUi(); }
    void addKnob(const juce::String& title, const juce::String& id);
    void fillPatchBox(int bank);
    void syncPatchUi();
    void stepPatch(int delta);

    SynthLookAndFeel laf;   // declared first so it outlives every child component
    NocturneDarkSynthAudioProcessor& processor;
    OscWindow sineWindow, triWindow, sawWindow;
    juce::OwnedArray<juce::Slider> knobs;
    juce::OwnedArray<juce::Label> labels;
    juce::ComboBox bankBox, patchBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    juce::MidiKeyboardComponent keyboard;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;

    juce::Rectangle<int> headerBar, lcdArea, filterPanel, ampPanel, phaserPanel, keyboardBar;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NocturneDarkSynthAudioProcessorEditor)
};
