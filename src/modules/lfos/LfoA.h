#pragma once
#include "engine/Module.h"
#include <cmath>

namespace synth {

// ── LfoA — oscylator wolnozmiennej częstotliwości ────────────────────────────
// Kształty: Sine/Triangle/Saw/Square/S&H
// Synchronizacja do tempa, retrigger po NoteOn
// Porty: brak wejść (poza opcjonalnym sync gate), wyjście CV [-1..1]
class LfoA : public Module {
public:
    enum Param {
        kRate,      // [0..1] → 0.05 Hz..20 Hz (log)
        kWaveform,  // 0=sin, 1=tri, 2=saw, 3=square, 4=S&H
        kDepth,     // [0..1] → skala wyjścia
        kNumParams
    };

    void prepare(double sampleRate, int blockSize) override;
    void reset() override { phase_ = 0.0f; sampleHold_ = 0.0f; }

    void  process(const float* const* inputs, float* const* outputs, int n) override;
    void  setParam(int i, float v) override;
    float getParam(int i) const override { return params_[i]; }
    int   getNumParams()  const override { return kNumParams; }

    std::vector<PortInfo> getPorts() const override;
    std::string           getName()  const override { return "LfoA"; }

private:
    double sampleRate_ = 96000.0;
    float  phase_      = 0.0f;
    float  sampleHold_ = 0.0f;  // wartość S&H
    bool   prevSync_   = false;
    float  params_[kNumParams] = { 0.3f, 0.0f, 1.0f };
};

} // namespace synth
