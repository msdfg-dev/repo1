#pragma once
#include <JuceHeader.h>
#include <atomic>
#include "DarkVoice.h"
#include "Presets.h"

class NocturneDarkSynthAudioProcessor : public juce::AudioProcessor
{
public:
    NocturneDarkSynthAudioProcessor();
    ~NocturneDarkSynthAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Nocturne Dark Synth"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }

    // Every factory patch is exposed to the host as a program.
    int getNumPrograms() override { return Presets::totalPatches(); }
    int getCurrentProgram() override { return currentProgram.load(); }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // Load a factory patch (overwrites all sound parameters).
    void applyPatch(int index);

    juce::AudioProcessorValueTreeState parameters;
    juce::MidiKeyboardState keyboardState;   // on-screen / computer keyboard -> synth

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void setParam(const char* id, float plainValue);

    juce::Synthesiser synth;
    juce::dsp::Phaser<float> phaser;
    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> cutoffSmooth;

    std::atomic<float>* cutoffParam = nullptr;
    std::atomic<float>* resonanceParam = nullptr;
    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* phaserParam = nullptr;
    std::atomic<float>* rateParam = nullptr;
    std::atomic<float>* depthParam = nullptr;

    std::atomic<int> currentProgram { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NocturneDarkSynthAudioProcessor)
};
