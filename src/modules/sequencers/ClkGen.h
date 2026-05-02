#pragma once
#include "engine/Module.h"
#include <cmath>

namespace synth {

// ── ClkGen — wewnętrzny generator zegara BPM ─────────────────────────────────
// Generuje impuls gate o zadanym BPM i podziale rytmicznym.
// Synchronizuje się z hostem (JUCE AudioPlayHead) gdy dostępny.
//
// Wyjścia:
//   [0] Clock Out   — gate 1/0 (impuls 10ms)
//   [1] Quarter Out — beat co ćwierćnutę
class ClkGen : public Module {
public:
    enum Param {
        kBPM,      // [0..1] → 40..240 BPM
        kDivision, // [0..1] → podział: 1/32, 1/16, 1/8, 1/4, 1/2, 1/1
        kNumParams
    };

    void prepare(double sampleRate, int blockSize) override;
    void reset()   override { phase_ = 0.0; beatPhase_ = 0.0; }

    void  process(const float* const* inputs,
                  float* const*       outputs,
                  int                 numSamples) override;

    void  setParam(int i, float v) override;
    float getParam(int i) const    override { return params_[i]; }
    int   getNumParams()  const    override { return kNumParams; }

    // Ustawianie BPM z zewnątrz (host sync)
    void setHostBPM(double bpm) { hostBPM_ = bpm; }

    std::vector<PortInfo> getPorts() const override;
    std::string           getName()  const override { return "ClkGen"; }

private:
    double sampleRate_ = 96000.0;
    double phase_      = 0.0;   // faza podziału [0..1)
    double beatPhase_  = 0.0;   // faza ćwierćnuty [0..1)
    double hostBPM_    = 0.0;   // 0 = brak hosta, używaj własnego BPM

    // Długość impulsu wyjściowego w próbkach (~10ms)
    int    pulseCounter_     = 0;
    int    beatPulseCounter_ = 0;

    static constexpr int kPulseLength = 960;  // 10ms @ 96kHz

    float params_[kNumParams] = { 0.5f, 0.5f };
    //                             120BPM  1/4
};

} // namespace synth
