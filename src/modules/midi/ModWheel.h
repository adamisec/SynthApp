#pragma once
#include "engine/Module.h"

namespace synth {

// ── ModWheel — źródło CV z kółka modulacji MIDI ──────────────────────────────
class ModWheel : public Module {
public:
    enum Param { kValue, kNumParams };

    void prepare(double sr, int bs) override { (void)sr; (void)bs; }
    void reset() override {}

    void  process(const float* const* inputs, float* const* outputs, int n) override;
    void  setParam(int i, float v) override;
    float getParam(int i) const override { return params_[i]; }
    int   getNumParams()  const override { return kNumParams; }

    std::vector<PortInfo> getPorts() const override;
    std::string           getName()  const override { return "ModWheel"; }

private:
    float params_[kNumParams] = { 0.0f };
};

} // namespace synth
