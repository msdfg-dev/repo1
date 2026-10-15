#pragma once
#include <JuceHeader.h>

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
    double getTailLengthSeconds() const override { return 2.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Nocturne"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;
    bool loadSample(const juce::File& file);
    juce::String getSampleName() const { return sampleName; }

private:
    struct DarkVoice : public juce::SynthesiserVoice
    {
        bool canPlaySound(juce::SynthesiserSound* sound) override;
        void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override;
        void stopNote(float velocity, bool allowTailOff) override;
        void pitchWheelMoved(int) override {}
        void controllerMoved(int, int) override {}
        void renderNextBlock(juce::AudioBuffer<float>&, int startSample, int numSamples) override;
        void setRate(double rate) { sampleRate = rate; }
        double sampleRate = 44100.0, phase = 0.0, phase2 = 0.0;
        float level = 0.0f, noteHz = 440.0f, velocityLevel = 0.0f, release = 0.0f;
        bool active = false, releasing = false;
    };
    struct DarkSound : public juce::SynthesiserSound
    {
        bool appliesToNote(int) override { return true; }
        bool appliesToChannel(int) override { return true; }
    };

    juce::Synthesiser synth;
    juce::AudioFormatManager formatManager;
    juce::Synthesiser sampler;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::dsp::Phaser<float> phaser;
    juce::dsp::IIR::Filter<float> lowpassL, lowpassR;
    juce::AudioBuffer<float> scratch;
    juce::String sampleName = "No sample loaded";
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NocturneDarkSynthAudioProcessor)
};
