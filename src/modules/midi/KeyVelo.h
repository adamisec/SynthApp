#pragma once
#include "engine/Module.h"

namespace synth {

// ── KeyVelo — MIDI Velocity → CV ─────────────────────────────────────────────
// Zamienia velocity nuty (0..127) na wartość CV [0..1].
// VoicePool ustawia velocity przez setParam() przed każdym blokiem.
//
// Wyjścia:
//   [0] Velocity CV  — 0..1
//   [1] Inv Velocity — 1 - velocity (do modulacji odwrotnej)
class KeyVelo : public Module {
public:
    enum Param {
        kVelocity,  // 0..1 (ustawiane przez VoicePool)
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
    std::string           getName()  const override { return "KeyVelo"; }

private:
    float params_[kNumParams] = { 0.8f };
};

} // namespace synth
