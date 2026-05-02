#pragma once
#include "engine/Module.h"
#include <cmath>

namespace synth {

// ── FltNord — ZDF State-Variable Filter ──────────────────────────────────────
// Charakterystyczny "Nord filter" — multimode: LP/HP/BP/Notch
// 12 lub 24 dB/okt, parametryczne Q
// Porty: [in0] Audio, [in1] Cutoff CV, [in2] Res CV
// Wyjścia: [out0] LP, [out1] HP, [out2] BP, [out3] Notch
class FltNord : public Module {
public:
    enum Param {
        kCutoff, kResonance, kSlope,  // 0=12dB, 1=24dB
        kNumParams
    };

    void prepare(double sampleRate, int blockSize) override;
    void reset() override;

    void  process(const float* const* inputs, float* const* outputs, int n) override;
    void  setParam(int i, float v) override;
    float getParam(int i) const override { return params_[i]; }
    int   getNumParams()  const override { return kNumParams; }

    std::vector<PortInfo> getPorts() const override;
    std::string           getName()  const override { return "FltNord"; }

private:
    double sampleRate_ = 96000.0;
    float ic1eq_ = 0.0f, ic2eq_ = 0.0f;  // SVF integrator states
    float params_[kNumParams] = { 0.6f, 0.2f, 0.0f };
};

} // namespace synth
