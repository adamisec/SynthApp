#pragma once
#include "engine/Module.h"

namespace synth {

// ── PitchBnd — MIDI Pitch Bend → CV ──────────────────────────────────────────
// Zamienia wartość pitch bend (14-bit, -8192..+8191) na CV [0..1].
// Środek = 0.5 (brak bendu). Zakres ±2 półtony (semitones) domyślnie.
// VoicePool/PluginProcessor ustawia wartość przez setParam().
//
// Wyjście:
//   [0] Bend CV — 0..1 (0.5 = środek = zero bendu)
class PitchBnd : public Module {
public:
    enum Param {
        kBend,   // 0..1 (0.5 = środek)
        kRange,  // [0..1] → ±1..±12 półtonów (0.5 = ±2)
        kNumParams
    };

    void prepare(double sampleRate, int blockSize) override { (void)sampleRate; (void)blockSize; }
    void reset()   override { params_[kBend] = 0.5f; }

    void  process(const float* const* inputs,
                  float* const*       outputs,
                  int                 numSamples) override;

    void  setParam(int i, float v) override;
    float getParam(int i) const    override { return params_[i]; }
    int   getNumParams()  const    override { return kNumParams; }

    std::vector<PortInfo> getPorts() const override;
    std::string           getName()  const override { return "PitchBnd"; }

private:
    float params_[kNumParams] = { 0.5f, 0.0f };
    //                             0bend  ±1st
};

} // namespace synth
