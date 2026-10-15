#include "PluginProcessor.h"
#include "PluginEditor.h"

NocturneDarkSynthAudioProcessor::NocturneDarkSynthAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    cutoffParam    = parameters.getRawParameterValue("cutoff");
    resonanceParam = parameters.getRawParameterValue("resonance");
    driveParam     = parameters.getRawParameterValue("drive");
    phaserParam    = parameters.getRawParameterValue("phaser");
    rateParam      = parameters.getRawParameterValue("rate");
    depthParam     = parameters.getRawParameterValue("depth");

    VoiceParams vp;
    vp.attack  = parameters.getRawParameterValue("attack");
    vp.release = parameters.getRawParameterValue("release");
    vp.wave    = parameters.getRawParameterValue("wave");
    vp.detune  = parameters.getRawParameterValue("detune");
    vp.sub     = parameters.getRawParameterValue("sub");

    synth.addSound(new DarkSound());
    for (int i = 0; i < 8; ++i)
        synth.addVoice(new DarkVoice(vp));

    applyPatch(0);
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
    p.push_back(std::make_unique<APF>("detune", "Detune", 0.f, 50.f, 6.f));
    p.push_back(std::make_unique<APF>("sub", "Sub Level", 0.f, 1.f, 0.6f));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("wave", "Waveform",
                    juce::StringArray { "Saw", "Square", "Triangle" }, 0));
    return { p.begin(), p.end() };
}

void NocturneDarkSynthAudioProcessor::setParam(const char* id, float plainValue)
{
    if (auto* p = parameters.getParameter(id))
        p->setValueNotifyingHost(p->convertTo0to1(plainValue));
}

void NocturneDarkSynthAudioProcessor::applyPatch(int index)
{
    index = juce::jlimit(0, Presets::totalPatches() - 1, index);
    const auto& patch = Presets::get(index);

    setParam("cutoff", patch.cutoff);
    setParam("resonance", patch.resonance);
    setParam("drive", patch.drive);
    setParam("phaser", patch.phaser);
    setParam("rate", patch.rate);
    setParam("depth", patch.depth);
    setParam("attack", patch.attack);
    setParam("release", patch.release);
    setParam("wave", (float) patch.wave);
    setParam("detune", patch.detune);
    setParam("sub", patch.sub);

    currentProgram = index;
    updateHostDisplay();
}

void NocturneDarkSynthAudioProcessor::setCurrentProgram(int index)
{
    // Hosts often re-send the current program after restoring state; ignore that
    // so it doesn't overwrite saved knob positions.
    if (index != currentProgram.load())
        applyPatch(index);
}

const juce::String NocturneDarkSynthAudioProcessor::getProgramName(int index)
{
    if (index < 0 || index >= Presets::totalPatches())
        return {};
    int bank = 0, patch = 0;
    Presets::locate(index, bank, patch);
    return juce::String(Presets::banks()[(size_t) bank].name) + " - " + Presets::get(index).name;
}

void NocturneDarkSynthAudioProcessor::prepareToPlay(double sr, int block)
{
    synth.setCurrentPlaybackSampleRate(sr);

    const auto numCh = (juce::uint32) juce::jmax(1, getTotalNumOutputChannels());
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) juce::jmax(1, block), numCh };

    phaser.prepare(spec);
    phaser.reset();
    phaser.setCentreFrequency(900.0f);
    phaser.setFeedback(0.55f);

    filter.prepare(spec);
    filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    filter.reset();

    cutoffSmooth.reset(sr, 0.02);
    cutoffSmooth.setCurrentAndTargetValue(cutoffParam->load());

    keyboardState.reset();
}

void NocturneDarkSynthAudioProcessor::releaseResources() {}

bool NocturneDarkSynthAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
        || l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void NocturneDarkSynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    buffer.clear();

    // Notes from the on-screen / computer keyboard join the host's MIDI.
    keyboardState.processNextMidiBuffer(midi, 0, n, true);

    synth.renderNextBlock(buffer, midi, 0, n);

    // Effects: phaser -> low-pass -> saturation.
    phaser.setRate(rateParam->load());
    phaser.setDepth(depthParam->load());
    phaser.setMix(phaserParam->load());

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> ctx(block);
    phaser.process(ctx);

    filter.setResonance(0.7f + resonanceParam->load());
    cutoffSmooth.setTargetValue(cutoffParam->load());
    const float gain = 1.0f + driveParam->load() * 6.0f;

    float* const* w = buffer.getArrayOfWritePointers();
    for (int i = 0; i < n; ++i)
    {
        filter.setCutoffFrequency(cutoffSmooth.getNextValue());
        for (int ch = 0; ch < numCh; ++ch)
            w[ch][i] = std::tanh(filter.processSample(ch, w[ch][i]) * gain);
    }
}

juce::AudioProcessorEditor* NocturneDarkSynthAudioProcessor::createEditor()
{
    return new NocturneDarkSynthAudioProcessorEditor(*this);
}

void NocturneDarkSynthAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = parameters.copyState();
    state.setProperty("program", currentProgram.load(), nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, dest);
}

void NocturneDarkSynthAudioProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
    {
        auto tree = juce::ValueTree::fromXml(*xml);
        const int program = (int) tree.getProperty("program", 0);
        parameters.replaceState(tree);
        currentProgram = juce::jlimit(0, Presets::totalPatches() - 1, program);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NocturneDarkSynthAudioProcessor();
}
