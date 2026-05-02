#pragma once
#include "engine/Module.h"
#include <cmath>

namespace synth {

// ── LfoB — LFO z kontrolą fazy startowej ─────────────────────────────────────
// Jak LfoA, ale z dodatkowym parametrem fazy startowej i wejściem sync.
// Wejścia:
//   [0] Sync In  — gate: reset fazy do kStartPhase
//   [1] Rate CV  — CV modulacja tempa
// Wyjścia:
//   [0] CV Out   — sygnał LFO [-1..1]
class LfoB : public Module {
public:
    enum Param {
        kRate,       // [0..1] → 0.05..20 Hz (log)
        kWaveform,   // 0=sin, 1=tri, 2=saw, 3=sqr, 4=S&H
        kDepth,      // [0..1]
        kStartPhase, // [0..1] → faza startowa 0°..360°
        kNumParams
    };

    void prepare(double sampleRate, int blockSize) override;
    void reset()   override;

    void  process(const float* const* inputs,
                  float* const*       outputs,
                  int                 numSamples) override;

    void  setParam(int i, float v) override;
    float getParam(int i) const    override { return params_[i]; }
    int   getNumParams()  const    override { return kNumParams; }

    std::vector<PortInfo> getPorts() const override;
    std::string           getName()  const override { return "LfoB"; }

private:
    double sampleRate_ = 96000.0;
    float  phase_      = 0.0f;
    float  sampleHold_ = 0.0f;
    bool   prevSync_   = false;
    float  params_[kNumParams] = { 0.3f, 0.0f, 1.0f, 0.0f };
};

} // namespace synth
