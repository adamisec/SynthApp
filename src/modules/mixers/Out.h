#pragma once
#include "engine/Module.h"

namespace synth {

// ── Out — stereo wyjście patcha ───────────────────────────────────────────────
// Sumuje do 2 wejść mono/stereo i wyprowadza na master L/R.
// Poziom: [0..1] → -inf..0 dBFS (log).
//
// Wejścia: [0] In L (lub mono), [1] In R (opcjonalnie)
// Wyjścia: [0] Out L, [1] Out R
class Out : public Module {
public:
    enum Param {
        kLevel,   // [0..1] → 0..1 amplitude (0.5 = -6 dBFS)
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
    std::string           getName()  const override { return "Out"; }

private:
    float params_[kNumParams] = { 0.8f };
};

} // namespace synth
