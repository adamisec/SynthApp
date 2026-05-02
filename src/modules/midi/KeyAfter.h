#pragma once
#include "engine/Module.h"

namespace synth {

// ── KeyAfter — MIDI Channel Aftertouch → CV ──────────────────────────────────
// Aftertouch (channel pressure) generuje CV [0..1].
// VoicePool ustawia wartość przez setParam(kAftertouch).
//
// Wyjścia:
//   [0] Aftertouch CV  — 0..1
//   [1] Inv Aftertouch — 1 - aftertouch
class KeyAfter : public Module {
public:
    enum Param {
        kAftertouch,  // 0..1 (ustawiane przez VoicePool)
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
    std::string           getName()  const override { return "KeyAfter"; }

private:
    float params_[kNumParams] = { 0.0f };
};

} // namespace synth
