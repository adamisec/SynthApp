#pragma once
#include "engine/Module.h"
#include <cmath>
#include <array>

namespace synth {

// ── FxEQ — 3-pasmowy equalizer parametryczny ─────────────────────────────────
// Pasma: Low Shelf | Mid Peak (Bell) | High Shelf
// Każde pasmo: częstotliwość, gain (±12dB), Q (tylko mid)
//
// Wejścia:
//   [0] Audio In
// Wyjścia:
//   [0] Audio Out
//
// Parametry:
//   kLowFreq   — Low shelf freq  [0..1] → 40..800 Hz
//   kLowGain   — Low shelf gain  [0..1] → -12..+12 dB
//   kMidFreq   — Mid bell freq   [0..1] → 200..8000 Hz
//   kMidGain   — Mid bell gain   [0..1] → -12..+12 dB
//   kMidQ      — Mid bell Q      [0..1] → 0.3..8.0
//   kHighFreq  — High shelf freq [0..1] → 2000..16000 Hz
//   kHighGain  — High shelf gain [0..1] → -12..+12 dB
class FxEQ : public Module {
public:
    enum Param {
        kLowFreq, kLowGain,
        kMidFreq, kMidGain, kMidQ,
        kHighFreq, kHighGain,
        kNumParams
    };

    void prepare(double sampleRate, int blockSize) override;
    void reset()   override;

    void  process(const float* const* inputs,
                  float* const*       outputs,
                  int                 numSamples) override;

    void  setParam(int i, float v) override;
    float getParam(int i) const    override { return params_[i]; }
    int   getNumParams()  const    override { return kNumParams; }

    std::vector<PortInfo> getPorts() const override;
    std::string           getName()  const override { return "FxEQ"; }

private:
    // Biquad: [b0 b1 b2 a1 a2]
    struct Biquad {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        float x1 = 0, x2 = 0, y1 = 0, y2 = 0;

        float tick(float x) {
            float y = b0*x + b1*x1 + b2*x2 - a1*y1 - a2*y2;
            x2 = x1; x1 = x;
            y2 = y1; y1 = y;
            return y;
        }
        void clear() { x1 = x2 = y1 = y2 = 0.0f; }
    };

    void computeCoeffs();

    // Low shelf
    void calcLowShelf(Biquad& bq, float freqHz, float gainDB, double sr);
    // High shelf
    void calcHighShelf(Biquad& bq, float freqHz, float gainDB, double sr);
    // Bell (peaking EQ)
    void calcBell(Biquad& bq, float freqHz, float gainDB, float Q, double sr);

    double sampleRate_ = 96000.0;
    Biquad lowBQ_, midBQ_, highBQ_;

    float params_[kNumParams] = {
        0.1f,  0.5f,   // low  freq=~100Hz, gain=0dB
        0.45f, 0.5f, 0.3f,  // mid  freq=~1kHz, gain=0dB, Q=1
        0.7f,  0.5f,   // high freq=~6kHz, gain=0dB
    };
};

} // namespace synth
