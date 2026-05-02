#pragma once
#include "engine/Module.h"
#include <cmath>

namespace synth {

// ── FxCompressor — kompresor szczytowy (feed-forward RMS) ───────────────────
// Detekcja: peak lub RMS (kDetect < 0.5 = peak)
// Gain Computer: threshold / ratio / knee
// Gain Smoother: attack / release (1-pole IIR)
// Makeup Gain automatyczny (opcja)
//
// Wejście:  [0] Audio In, [1] Sidechain (opcja)
// Wyjście:  [0] Audio Out, [1] GR CV (gain reduction, 0..−1)
class FxCompressor : public Module {
public:
    enum Param {
        kThreshold, // [0..1] → -60..0 dBFS
        kRatio,     // [0..1] → 1:1..∞:1 (log)
        kAttack,    // [0..1] → 0.1ms..100ms (log)
        kRelease,   // [0..1] → 5ms..2000ms (log)
        kMakeup,    // [0..1] → 0..+24 dB makeup gain
        kKnee,      // [0..1] → szerokość soft-knee 0..12 dB
        kDetect,    // <0.5 = peak, ≥0.5 = RMS
        kNumParams
    };

    void prepare(double sampleRate, int blockSize) override;
    void reset()   override { envFollower_ = 0.0f; gainSmooth_ = 1.0f; rmsAccum_ = 0.0f; }

    void  process(const float* const* inputs,
                  float* const*       outputs,
                  int                 numSamples) override;

    void  setParam(int i, float v) override;
    float getParam(int i) const    override { return params_[i]; }
    int   getNumParams()  const    override { return kNumParams; }

    std::vector<PortInfo> getPorts() const override;
    std::string           getName()  const override { return "FxCompressor"; }

private:
    // Gain computer (returns desired gain in linear)
    float computeGain(float levelDb) const noexcept;

    static float linToDb(float x) noexcept {
        return 20.0f * std::log10(std::max(x, 1e-9f));
    }
    static float dbToLin(float db) noexcept {
        return std::pow(10.0f, db * 0.05f);
    }

    double sampleRate_  = 96000.0;
    float  envFollower_ = 0.0f;   // peak/RMS envelope
    float  gainSmooth_  = 1.0f;   // wygładzony gain (liniowy)
    float  rmsAccum_    = 0.0f;   // akumulator RMS

    // Współczynniki attack/release (przeliczane w setParam)
    float  attCoeff_    = 0.0f;
    float  relCoeff_    = 0.0f;

    float params_[kNumParams] = { 0.75f, 0.4f, 0.2f, 0.4f, 0.5f, 0.3f, 0.0f };
    //                             -15dB  ~4:1  ~3ms  ~50ms  +12dB  6dB  peak
};

} // namespace synth
