#pragma once
#include <vector>

// Factory sound banks. Each patch is a full set of synth parameter values.
namespace Presets
{
    struct Patch
    {
        const char* name;
        float cutoff, resonance, drive, phaser, rate, depth, attack, release;
        int wave;          // 0 = saw, 1 = square, 2 = triangle
        float detune;      // cents between the two oscillators
        float sub;         // sub-oscillator level 0..1
    };

    struct Bank
    {
        const char* name;
        std::vector<Patch> patches;
    };

    inline const std::vector<Bank>& banks()
    {
        static const std::vector<Bank> b = {
            { "NOCTURNE", {
                { "Midnight Pad", 1400, 0.35f, 0.20f, 0.45f, 0.25f, 0.60f, 0.90f, 2.2f, 0,  9.f, 0.60f },
                { "Black Glass",  3200, 0.50f, 0.15f, 0.30f, 0.40f, 0.50f, 0.02f, 0.9f, 0, 12.f, 0.30f },
                { "Dusk Bass",     520, 0.70f, 0.45f, 0.10f, 0.20f, 0.30f, 0.005f, 0.25f, 0, 4.f, 0.90f },
                { "Hollow Keys",  2600, 0.30f, 0.10f, 0.20f, 0.50f, 0.40f, 0.005f, 0.70f, 2, 3.f, 0.40f },
                { "Eclipse Lead", 4200, 0.60f, 0.35f, 0.25f, 0.80f, 0.35f, 0.01f, 0.40f, 1, 8.f, 0.20f },
            } },
            { "ABYSS", {
                { "Deep Drone",    300, 0.50f, 0.50f, 0.50f, 0.10f, 0.80f, 1.80f, 4.5f, 0, 20.f, 1.00f },
                { "Sunken Choir",  900, 0.40f, 0.10f, 0.70f, 0.15f, 0.90f, 1.40f, 3.5f, 2, 15.f, 0.50f },
                { "Pressure",      220, 0.90f, 0.60f, 0.20f, 0.05f, 0.40f, 0.30f, 2.0f, 1,  6.f, 0.80f },
                { "Trench Pulse",  700, 1.00f, 0.55f, 0.55f, 1.20f, 0.70f, 0.003f, 0.35f, 1, 10.f, 0.70f },
                { "Black Water",  1100, 0.45f, 0.25f, 0.80f, 0.20f, 1.00f, 0.60f, 3.0f, 0, 25.f, 0.50f },
            } },
            { "STATIC", {
                { "Neon Rust",    5200, 0.55f, 0.50f, 0.40f, 2.50f, 0.60f, 0.005f, 0.45f, 0, 18.f, 0.10f },
                { "Wire Saw",     7600, 0.80f, 0.70f, 0.10f, 0.30f, 0.20f, 0.001f, 0.20f, 0, 30.f, 0.00f },
                { "Cold Arp",     3600, 0.50f, 0.20f, 0.30f, 3.00f, 0.50f, 0.001f, 0.18f, 1,  5.f, 0.20f },
                { "Glitch Bell",  6200, 1.10f, 0.15f, 0.60f, 6.00f, 0.80f, 0.001f, 1.20f, 2,  2.f, 0.00f },
                { "Broken Radio", 2200, 0.65f, 0.90f, 0.90f, 4.50f, 1.00f, 0.01f, 0.50f, 0, 45.f, 0.30f },
            } },
        };
        return b;
    }

    inline int totalPatches()
    {
        int n = 0;
        for (auto& b : banks()) n += (int) b.patches.size();
        return n;
    }

    inline int indexOf(int bank, int patch)
    {
        int idx = 0;
        for (int i = 0; i < bank; ++i) idx += (int) banks()[(size_t) i].patches.size();
        return idx + patch;
    }

    inline void locate(int index, int& bank, int& patch)
    {
        bank = 0;
        for (auto& b : banks())
        {
            if (index < (int) b.patches.size()) { patch = index; return; }
            index -= (int) b.patches.size();
            ++bank;
        }
        bank = 0; patch = 0;
    }

    inline const Patch& get(int index)
    {
        int bank = 0, patch = 0;
        locate(index, bank, patch);
        return banks()[(size_t) bank].patches[(size_t) patch];
    }
}
