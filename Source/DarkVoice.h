#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <cmath>
#include "Waveforms.h"

// Sound object that accepts every note/channel; the oscillator voices play it.
struct DarkSound : public juce::SynthesiserSound
{
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

// Parameters of one oscillator window.
struct OscParams
{
    std::atomic<float>* level = nullptr;   // 0..1
    std::atomic<float>* octave = nullptr;  // -2..+2
    std::atomic<float>* fine = nullptr;    // cents
    std::atomic<float>* shape = nullptr;   // 0..1, meaning depends on the oscillator
};

struct VoiceParams
{
    std::atomic<float>* attack = nullptr;   // seconds
    std::atomic<float>* release = nullptr;  // seconds
    OscParams sine, tri, saw;
};

// Three oscillators (sine, triangle, saw) mixed together and shaped by an ADSR.
class DarkVoice : public juce::SynthesiserVoice
{
public:
    explicit DarkVoice(const VoiceParams& p) : params(p) {}

    bool canPlaySound(juce::SynthesiserSound* s) override
    {
        return dynamic_cast<DarkSound*>(s) != nullptr;
    }

    void startNote(int midiNote, float velocity, juce::SynthesiserSound*, int pitchWheel) override
    {
        baseHz = juce::MidiMessage::getMidiNoteInHertz(midiNote);
        bendSemitones = (pitchWheel - 8192) / 8192.0 * 2.0;
        velGain = 0.25f + 0.75f * velocity;
        phaseSine = 0.0;
        phaseTri = 0.25;
        phaseSaw = 0.5;
        updatePitch();
        adsr.setSampleRate(getSampleRate() > 0.0 ? getSampleRate() : 44100.0);
        updateEnvelope();
        adsr.noteOn();
    }

    void stopNote(float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            updateEnvelope();
            adsr.noteOff();
        }
        else
        {
            adsr.reset();
            clearCurrentNote();
        }
    }

    void pitchWheelMoved(int newValue) override
    {
        bendSemitones = (newValue - 8192) / 8192.0 * 2.0;
        updatePitch();
    }

    void controllerMoved(int, int) override {}

    void renderNextBlock(juce::AudioBuffer<float>& out, int startSample, int numSamples) override
    {
        const double sr = getSampleRate();
        if (sr <= 0.0 || ! isVoiceActive())
            return;

        const float lvSine = level(params.sine), lvTri = level(params.tri), lvSaw = level(params.saw);
        const float shSine = shape(params.sine), shTri = shape(params.tri), shSaw = shape(params.saw);
        const double dtSine = increment(params.sine, sr);
        const double dtTri = increment(params.tri, sr);
        const double dtSaw = increment(params.saw, sr);

        // Keep the combined level sensible however many oscillators are up.
        const float total = lvSine + lvTri + lvSaw;
        const float gain = 0.34f / juce::jmax(1.0f, total) * velGain;

        const int numChannels = out.getNumChannels();

        for (int i = 0; i < numSamples; ++i)
        {
            const float env = adsr.getNextSample();

            float s = 0.0f;
            if (lvSine > 0.0005f) s += lvSine * Wave::sine(phaseSine, shSine);
            if (lvTri > 0.0005f)  s += lvTri * Wave::triangle(phaseTri, shTri);
            if (lvSaw > 0.0005f)  s += lvSaw * Wave::saw(phaseSaw, shSaw, dtSaw);
            s *= env * gain;

            for (int ch = 0; ch < numChannels; ++ch)
                out.addSample(ch, startSample + i, s);

            phaseSine += dtSine; if (phaseSine >= 1.0) phaseSine -= 1.0;
            phaseTri += dtTri;   if (phaseTri >= 1.0)  phaseTri -= 1.0;
            phaseSaw += dtSaw;   if (phaseSaw >= 1.0)  phaseSaw -= 1.0;

            if (! adsr.isActive())
            {
                clearCurrentNote();
                break;
            }
        }
    }

private:
    static float load(const std::atomic<float>* a, float fallback)
    {
        return a != nullptr ? a->load() : fallback;
    }

    static float level(const OscParams& o) { return juce::jlimit(0.0f, 1.0f, load(o.level, 0.0f)); }
    static float shape(const OscParams& o) { return juce::jlimit(0.0f, 1.0f, load(o.shape, 0.0f)); }

    // Phase increment per sample for one oscillator (octave + fine tune applied).
    double increment(const OscParams& o, double sr) const
    {
        const double oct = std::round((double) load(o.octave, 0.0f));
        const double cents = (double) load(o.fine, 0.0f);
        const double f = hz * std::pow(2.0, oct + cents / 1200.0);
        return juce::jlimit(0.0, 0.45, f / sr);
    }

    void updatePitch() { hz = baseHz * std::pow(2.0, bendSemitones / 12.0); }

    void updateEnvelope()
    {
        juce::ADSR::Parameters p;
        p.attack = juce::jmax(0.001f, load(params.attack, 0.04f));
        p.decay = 0.1f;
        p.sustain = 1.0f;
        p.release = juce::jmax(0.02f, load(params.release, 0.8f));
        adsr.setParameters(p);
    }

    VoiceParams params;
    juce::ADSR adsr;
    double baseHz = 440.0, hz = 440.0, bendSemitones = 0.0;
    double phaseSine = 0.0, phaseTri = 0.25, phaseSaw = 0.5;
    float velGain = 1.0f;
};
