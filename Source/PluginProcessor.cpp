#include "PluginProcessor.h"
#include "PluginEditor.h"

NocturneDarkSynthAudioProcessor::NocturneDarkSynthAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    formatManager.registerBasicFormats();
    synth.addSound(new DarkSound());
    for (int i = 0; i < 8; ++i) synth.addVoice(new DarkVoice());
    sampler.addSound(new DarkSound());
    for (int i = 0; i < 8; ++i) sampler.addVoice(new juce::SamplerVoice());
}

juce::AudioProcessorValueTreeState::ParameterLayout NocturneDarkSynthAudioProcessor::createParameterLayout()
{
    using APF = juce::AudioParameterFloat;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<APF>("cutoff", "Filter Cutoff", juce::NormalisableRange<float>(80.f, 12000.f, 1.f, 0.35f), 1800.f));
    p.push_back(std::make_unique<APF>("resonance", "Resonance", 0.1f, 1.2f, 0.35f));
    p.push_back(std::make_unique<APF>("drive", "Drive", 0.f, 1.f, 0.22f));
    p.push_back(std::make_unique<APF>("phaser", "Phaser Mix", 0.f, 1.f, 0.35f));
    p.push_back(std::make_unique<APF>("rate", "LFO Rate", 0.05f, 8.f, 0.32f));
    p.push_back(std::make_unique<APF>("depth", "Mod Depth", 0.f, 1.f, 0.55f));
    p.push_back(std::make_unique<APF>("attack", "Attack", 0.001f, 2.f, 0.04f));
    p.push_back(std::make_unique<APF>("release", "Release", 0.02f, 5.f, 0.8f));
    p.push_back(std::make_unique<APF>("samplemix", "Sampler Mix", 0.f, 1.f, 0.5f));
    return { p.begin(), p.end() };
}

bool NocturneDarkSynthAudioProcessor::DarkVoice::canPlaySound(juce::SynthesiserSound* s)
{ return dynamic_cast<DarkSound*>(s) != nullptr; }

void NocturneDarkSynthAudioProcessor::DarkVoice::startNote(int note, float vel, juce::SynthesiserSound*, int)
{
    noteHz = (float)juce::MidiMessage::getMidiNoteInHertz(note);
    velocityLevel = vel; phase = phase2 = 0.0; level = 0.0f; release = 0.0f;
    active = true; releasing = false;
}
void NocturneDarkSynthAudioProcessor::DarkVoice::stopNote(float, bool tail)
{
    if (tail) releasing = true;
    else { active = false; clearCurrentNote(); }
}
void NocturneDarkSynthAudioProcessor::DarkVoice::renderNextBlock(juce::AudioBuffer<float>& b, int start, int n)
{
    if (!active) return;
    const double inc = juce::MathConstants<double>::twoPi * noteHz / sampleRate;
    for (int i = 0; i < n; ++i)
    {
        // Detuned saw + sub sine + slow moving pulse: original raw oscillator source.
        float saw = (float)(2.0 * (phase / juce::MathConstants<double>::twoPi) - 1.0);
        float sub = (float)std::sin(phase2);
        float pulse = std::sin(phase * 0.503) > 0.0 ? 0.22f : -0.22f;
        float env = releasing ? (level *= 0.9993f) : (level = juce::jmin(1.0f, level + 0.0018f));
        float s = (0.58f * saw + 0.26f * sub + 0.16f * pulse) * velocityLevel * env * 0.16f;
        for (int ch = 0; ch < b.getNumChannels(); ++ch) b.addSample(ch, start + i, s);
        phase += inc; phase2 += inc * 0.5;
        if (phase >= juce::MathConstants<double>::twoPi) phase -= juce::MathConstants<double>::twoPi;
        if (phase2 >= juce::MathConstants<double>::twoPi) phase2 -= juce::MathConstants<double>::twoPi;
        if (releasing && level < 0.0005f) { active = false; clearCurrentNote(); break; }
    }
}

void NocturneDarkSynthAudioProcessor::prepareToPlay(double sr, int block)
{
    synth.setCurrentPlaybackSampleRate(sr);
    sampler.setCurrentPlaybackSampleRate(sr);
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<DarkVoice*>(synth.getVoice(i))) v->setRate(sr);
    juce::dsp::ProcessSpec spec { sr, (juce::uint32)block, (juce::uint32)getTotalNumOutputChannels() };
    phaser.prepare(spec);
    phaser.setRate(*parameters.getRawParameterValue("rate"));
    phaser.setDepth(*parameters.getRawParameterValue("depth"));
    phaser.setCentreFrequency(900.0f);
    phaser.setFeedback(0.55f);
    phaser.setMix(*parameters.getRawParameterValue("phaser"));
    lowpassL.reset(); lowpassR.reset();
    auto c = juce::dsp::IIR::Coefficients<float>::makeLowPass(sr, 1800.0);
    lowpassL.coefficients = c; lowpassR.coefficients = c;
}
void NocturneDarkSynthAudioProcessor::releaseResources() {}
bool NocturneDarkSynthAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{ return l.getMainOutputChannelSet() == juce::AudioChannelSet::mono() || l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }

void NocturneDarkSynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    synth.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());
    sampler.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());

    const float cutoff = parameters.getRawParameterValue("cutoff")->load();
    const float res = parameters.getRawParameterValue("resonance")->load();
    const float drive = parameters.getRawParameterValue("drive")->load();
    const float mix = parameters.getRawParameterValue("phaser")->load();
    const float rate = parameters.getRawParameterValue("rate")->load();
    const float depth = parameters.getRawParameterValue("depth")->load();
    auto coeff = juce::dsp::IIR::Coefficients<float>::makeLowPass(getSampleRate(), cutoff, 0.7f + res);
    lowpassL.coefficients = coeff; lowpassR.coefficients = coeff;
    phaser.setRate(rate); phaser.setDepth(depth); phaser.setMix(mix);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> ctx(block);
    phaser.process(ctx);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& filter = (ch == 0 ? lowpassL : lowpassR);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float x = filter.processSample(data[i]);
            data[i] = std::tanh(x * (1.0f + drive * 8.0f));
        }
    }
}

bool NocturneDarkSynthAudioProcessor::loadSample(const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (!reader) return false;
    juce::BigInteger notes; notes.setRange(0, 128, true);
    sampler.clearSounds();
    sampler.addSound(new juce::SamplerSound(file.getFileNameWithoutExtension(), *reader, notes, 60, 0.01, 1.0, 20.0));
    sampleName = file.getFileName();
    return true;
}

juce::AudioProcessorEditor* NocturneDarkSynthAudioProcessor::createEditor()
{ return new NocturneDarkSynthAudioProcessorEditor(*this); }

void NocturneDarkSynthAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{ auto state = parameters.copyState(); std::unique_ptr<juce::XmlElement> xml(state.createXml()); copyXmlToBinary(*xml, dest); }
void NocturneDarkSynthAudioProcessor::setStateInformation(const void* data, int size)
{ std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size)); if (xml && xml->hasTagName(parameters.state.getType())) parameters.replaceState(juce::ValueTree::fromXml(*xml)); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{ return new NocturneDarkSynthAudioProcessor(); }
