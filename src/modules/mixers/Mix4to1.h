#pragma once
#include "engine/Module.h"

namespace synth {

// ── Mix4to1 — mikser 4-do-1 ──────────────────────────────────────────────────
class Mix4to1 : public Module {
public:
    enum Param { kLevelA, kLevelB, kLevelC, kLevelD, kNumParams };

    void prepare(double sr, int bs) override { (void)sr; (void)bs; }
    void reset() override {}

    void  process(const float* const* inputs, float* const* outputs, int n) override;
    void  setParam(int i, float v) override;
    float getParam(int i) const override { return params_[i]; }
    int   getNumParams()  const override { return kNumParams; }

    std::vector<PortInfo> getPorts() const override;
    std::string           getName()  const override { return "Mix4to1"; }

private:
    float params_[kNumParams] = { 0.8f, 0.8f, 0.8f, 0.8f };
};

} // namespace synth
