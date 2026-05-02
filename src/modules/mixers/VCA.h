#pragma once
#include "engine/Module.h"

namespace synth {

// ── VCA — wzmacniacz sterowany napięciem ─────────────────────────────────────
// Bramka sygnału audio przez CV (typowo przez kopertę).
// Porty:  [in0] Audio In, [in1] CV (wzmocnienie), [out0] Audio Out
// Param:  kGain — poziom wzmocnienia (bez CV), kMode — lin/exp

class VCA : public Module {
public:
    enum Param { kGain, kMode, kNumParams };

    void prepare(double sampleRate, int blockSize) override { (void)sampleRate; (void)blockSize; }
    void reset()   override {}

    void  process(const float* const* inputs,
                  float* const*       outputs,
                  int                 numSamples) override;

    void  setParam(int i, float v) override;
    float getParam(int i) const    override { return params_[i]; }
    int   getNumParams()  const    override { return kNumParams; }

    std::vector<PortInfo> getPorts() const override;
    std::string           getName()  const override { return "VCA"; }

private:
    float params_[kNumParams] = { 1.0f, 0.0f };  // gain=max, linear
};

} // namespace synth
