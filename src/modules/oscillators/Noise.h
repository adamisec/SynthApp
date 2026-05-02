#pragma once
#include "engine/Module.h"
#include <cstdint>

namespace synth {

// ── Noise — generator szumu biały/różowy ─────────────────────────────────────
// Biały: równomierny rozkład widmowy
// Różowy: -3 dB/okt (Paul Kellet's filter chain)
class Noise : public Module {
public:
    enum Param { kColor, kLevel, kNumParams };  // kColor: 0=biały, 1=różowy

    void prepare(double sampleRate, int blockSize) override { (void)sampleRate; (void)blockSize; }
    void reset() override { seed_ = 12345; b0_=b1_=b2_=b3_=b4_=b5_=b6_=0.0f; }

    void  process(const float* const* inputs, float* const* outputs, int n) override;
    void  setParam(int i, float v) override;
    float getParam(int i) const override { return params_[i]; }
    int   getNumParams()  const override { return kNumParams; }

    std::vector<PortInfo> getPorts() const override;
    std::string           getName()  const override { return "Noise"; }

private:
    float nextWhite();

    uint32_t seed_ = 12345;
    float b0_=0,b1_=0,b2_=0,b3_=0,b4_=0,b5_=0,b6_=0;  // pink filter state
    float params_[kNumParams] = { 0.0f, 1.0f };
};

} // namespace synth
