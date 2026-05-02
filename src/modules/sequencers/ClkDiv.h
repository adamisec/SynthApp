#pragma once
#include "engine/Module.h"
#include <cmath>

namespace synth {

// ── ClkDiv — dzielnik zegara ──────────────────────────────────────────────────
// Wejście: Clock In (gate)
// Wyjścia: /2, /4, /8, /16, /32 (gate)
// Każde wyjście daje impuls co N impulsów wejściowych.
class ClkDiv : public Module {
public:
    enum Param {
        kNumParams = 0  // brak parametrów
    };

    void prepare(double sampleRate, int blockSize) override;
    void reset()   override;

    void  process(const float* const* inputs,
                  float* const*       outputs,
                  int                 numSamples) override;

    void  setParam(int i, float v) override { (void)i; (void)v; }
    float getParam(int i) const    override { (void)i; return 0.0f; }
    int   getNumParams()  const    override { return kNumParams; }

    std::vector<PortInfo> getPorts() const override;
    std::string           getName()  const override { return "ClkDiv"; }

private:
    double sampleRate_   = 96000.0;
    bool   prevClock_    = false;
    int    counter_      = 0;

    // Countery dla każdego podziału
    int   counts_[5]   = {};  // /2, /4, /8, /16, /32
    int   pulses_[5]   = {};  // liczniki impulsów wyjściowych

    static constexpr int kPulseLength = 960;  // 10ms @ 96kHz
    static constexpr int kDivs[5]     = { 2, 4, 8, 16, 32 };
};

} // namespace synth
