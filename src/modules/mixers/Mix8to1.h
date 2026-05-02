#pragma once
#include "engine/Module.h"

namespace synth {

// ── Mix8to1 — mikser 8 wejść → 1 wyjście ────────────────────────────────────
// Każde wejście ma niezależny poziom głośności.
// Wejścia:
//   [0..7] — kanały audio/CV
// Wyjścia:
//   [0]    — suma ważona
//
// Parametry:
//   kLevel0..kLevel7 — poziomy kanałów [0..1]
class Mix8to1 : public Module {
public:
    enum Param {
        kLevel0, kLevel1, kLevel2, kLevel3,
        kLevel4, kLevel5, kLevel6, kLevel7,
        kNumParams
    };

    void prepare(double sampleRate, int blockSize) override { (void)sampleRate; (void)blockSize; }
    void reset()   override {}

    void  process(const float* const* inputs,
                  float* const*       outputs,
                  int                 numSamples) override;

    void  setParam(int i, float v) override;
    float getParam(int i) const    override { return params_[i]; }
    int   getNumParams()  const    override { return kNumParams; }

    std::vector<PortInfo> getPorts() const override;
    std::string           getName()  const override { return "Mix8to1"; }

private:
    float params_[kNumParams] = { 0.75f, 0.75f, 0.75f, 0.75f,
                                  0.75f, 0.75f, 0.75f, 0.75f };
};

} // namespace synth
