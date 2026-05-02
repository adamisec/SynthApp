#pragma once
#include "engine/Module.h"
#include <algorithm>
#include <cmath>

namespace synth {

// ── OscD — oscylator FM (Chowning 1973, styl DX7) ────────────────────────────
// Carrier + Modulator w jednym module.
// Algorytm: carrier_out = sin(2π * fc * t + index * sin(2π * fm * t + fb))
//
// Wejście CV: [0] Pitch CV (1V/oct na carrier), [1] Index CV
// Wyjście:    [0] Audio out
class OscD : public Module {
public:
    enum Param {
        kPitch,    // [0..1] → MIDI 0..127 (częstotliwość carrier)
        kFine,     // [0..1] → ±1 semitonu
        kRatio,    // [0..1] → stosunek fm/fc: 0.5, 1, 2, 3, 4, 7 (6 pozycji)
        kIndex,    // [0..1] → głębokość modulacji 0..10
        kFeedback, // [0..1] → feedback modulatora 0..1 (self-FM)
        kNumParams
    };

    void prepare(double sampleRate, int blockSize) override;
    void reset()   override { phaseC_ = 0.0f; phaseM_ = 0.0f; lastMod_ = 0.0f; }

    void  process(const float* const* inputs,
                  float* const*       outputs,
                  int                 numSamples) override;

    void  setParam(int i, float v) override;
    float getParam(int i) const    override { return params_[i]; }
    int   getNumParams()  const    override { return kNumParams; }

    std::vector<PortInfo> getPorts() const override;
    std::string           getName()  const override { return "OscD"; }

private:
    static float midiToHz(float midi) noexcept {
        return 440.0f * std::exp2((midi - 69.0f) / 12.0f);
    }

    // Kwantyzowane wartości ratio (muzycznie sensowne)
    static float ratioFromParam(float p) noexcept {
        const float ratios[] = { 0.5f, 1.0f, 2.0f, 3.0f, 4.0f, 7.0f };
        int idx = static_cast<int>(p * 5.99f);
        return ratios[std::clamp(idx, 0, 5)];
    }

    double sampleRate_ = 96000.0;
    float  phaseC_     = 0.0f;  // faza carrier
    float  phaseM_     = 0.0f;  // faza modulator
    float  lastMod_    = 0.0f;  // poprzednia wartość modulator (dla feedback)

    float params_[kNumParams] = { 0.5f, 0.5f, 0.2f, 0.3f, 0.0f };
    //                             C4    0det  ratio=1  idx=3  no-fb
};

} // namespace synth
