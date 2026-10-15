#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <cmath>

// Sound object that accepts every note/channel; the oscillator voices play it.
struct DarkSound : public juce::SynthesiserSound
{
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

// Pointers to the plugin parameters a voice reads.
struct VoiceParams
{
    std::atomic<float>* attack = nullptr;   // seconds
    std::atomic<float>* release = nullptr;  // seconds
    std::atomic<float>* wave = nullptr;     // 0 saw, 1 square, 2 triangle
    std::atomic<float>* detune = nullptr;   // cents
    std::atomic<float>* sub = nullptr;      // 0..1
};

// Two detuned band-limited oscillators plus a sub sine one octave down,
// shaped by an ADSR driven by the Attack/Release parameters.
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
        phaseA = 0.0;
        phaseB = 0.25;
        phaseSub = 0.0;
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

        const int wave = juce::jlimit(0, 2, (int) std::lround(load(params.wave, 0.0f)));
        const double cents = load(params.detune, 6.0f);
        const float subLevel = 0.5f * juce::jlimit(0.0f, 1.0f, load(params.sub, 0.6f));
        const float waveGain = wave == 1 ? 0.75f : 1.0f;   // square is louder than saw

        const int numChannels = out.getNumChannels();
        const double dt = hz / sr;
        const double dtA = dt * std::pow(2.0, cents / 1200.0);
        const double dtB = dt * std::pow(2.0, -cents / 1200.0);
        const double dtSub = dt * 0.5;

        for (int i = 0; i < numSamples; ++i)
        {
            const float env = adsr.getNextSample();

            const float oscA = osc(wave, phaseA, dtA);
            const float oscB = osc(wave, phaseB, dtB);
            const float sub = (float) std::sin(juce::MathConstants<double>::twoPi * phaseSub);

            const float s = (0.42f * waveGain * (oscA + oscB) + subLevel * sub)
                            * env * velGain * voiceGain;

            for (int ch = 0; ch < numChannels; ++ch)
                out.addSample(ch, startSample + i, s);

            phaseA += dtA;     if (phaseA >= 1.0) phaseA -= 1.0;
            phaseB += dtB;     if (phaseB >= 1.0) phaseB -= 1.0;
            phaseSub += dtSub; if (phaseSub >= 1.0) phaseSub -= 1.0;

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

    static double polyBlep(double t, double dt)
    {
        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.0;
        }
        if (t > 1.0 - dt)
        {
            t = (t - 1.0) / dt;
            return t * t + t + t + 1.0;
        }
        return 0.0;
    }

    static float osc(int wave, double phase, double dt)
    {
        switch (wave)
        {
            case 1:   // square, band-limited
            {
                double v = phase < 0.5 ? 1.0 : -1.0;
                v += polyBlep(phase, dt);
                v -= polyBlep(std::fmod(phase + 0.5, 1.0), dt);
                return (float) v;
            }
            case 2:   // triangle
                return (float) (4.0 * std::abs(phase - 0.5) - 1.0);
            default:  // saw, band-limited
                return (float) (2.0 * phase - 1.0 - polyBlep(phase, dt));
        }
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
    double phaseA = 0.0, phaseB = 0.25, phaseSub = 0.0;
    float velGain = 1.0f;
    static constexpr float voiceGain = 0.26f;
};
