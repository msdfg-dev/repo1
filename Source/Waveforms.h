#pragma once
#include <cmath>

// Oscillator shapes shared by the audio voice and the UI waveform displays.
// `phase` is 0..1. Pass dt > 0 (phase increment per sample) to band-limit the
// edges with PolyBLEP; pass dt = 0 for plain drawing.
namespace Wave
{
    inline double blep(double t, double dt)
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

    // Sine; shape 0..1 adds harmonics by phase-modulating the sine with itself.
    inline float sine(double phase, float shape)
    {
        const double x = 6.283185307179586 * phase;
        return (float) std::sin(x + (double) shape * 3.0 * std::sin(x));
    }

    // Triangle; shape 0..1 moves the peak so the wave skews from triangle to ramp.
    inline float triangle(double phase, float shape)
    {
        const double pk = 0.5 + 0.49 * (double) shape;
        return (float) (phase < pk ? -1.0 + 2.0 * phase / pk
                                   :  1.0 - 2.0 * (phase - pk) / (1.0 - pk));
    }

    // Saw; shape 0..1 crossfades from saw to square.
    inline float saw(double phase, float shape, double dt)
    {
        double s = 2.0 * phase - 1.0;
        if (dt > 0.0) s -= blep(phase, dt);
        if (shape <= 0.0f) return (float) s;

        double sq = phase < 0.5 ? 1.0 : -1.0;
        if (dt > 0.0) sq += blep(phase, dt) - blep(std::fmod(phase + 0.5, 1.0), dt);
        return (float) ((1.0 - shape) * s + shape * 0.8 * sq);
    }
}
