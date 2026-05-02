#pragma once
#include "engine/Module.h"

namespace synth {

// ── Pan — panorama stereo ─────────────────────────────────────────────────────
// Equal-power panning (sin/cos law)
// Porty: [in0] Audio Mono, [in1] Pan CV; [out0] Left, [out1] Right
class Pan : public Module {
public:
    enum Param { kPan, kNumParams };  // 0=lewo, 0.5=środek, 1=prawo

    void prepare(double sr, int bs) override { (void)sr; (void)bs; }
    void reset() override {}

    void  process(const float* const* inputs, float* const* outputs, int n) override;
    void  setParam(int i, float v) override;
    float getParam(int i) const override { return params_[i]; }
    int   getNumParams()  const override { return kNumParams; }

    std::vector<PortInfo> getPorts() const override;
    std::string           getName()  const override { return "Pan"; }

private:
    float params_[kNumParams] = { 0.5f };
};

} // namespace synth
