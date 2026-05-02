#pragma once
#include "engine/Module.h"
#include <cmath>

namespace synth {

// ── EnvFollow — śledzenie obwiedni (RMS + Peak) ───────────────────────────────
// Mierzy poziom sygnału audio i generuje CV odpowiadające obwiedni.
// Wejścia:
//   [0] Audio In  — sygnał wejściowy
// Wyjścia:
//   [0] Env CV    — wolnozmienne CV [0..1] odpowiadające poziomowi
//
// Parametry:
//   kAttack   — czas ataku   [0..1] → 0.1ms..500ms
//   kRelease  — czas opadania[0..1] → 10ms..5000ms
//   kMode     — 0=RMS, 1=Peak
//   kGain     — wzmocnienie wejścia [0..1] → 0..+24dB
class EnvFollow : public Module {
public:
    enum Param {
        kAttack,
        kRelease,
        kMode,   // 0=RMS, 1=Peak
        kGain,
        kNumParams
    };

    void prepare(double sampleRate, int blockSize) override;
    void reset()   override { env_ = 0.0f; rmsAcc_ = 0.0f; }

    void  process(const float* const* inputs,
                  float* const*       outputs,
                  int                 numSamples) override;

    void  setParam(int i, float v) override;
    float getParam(int i) const    override { return params_[i]; }
    int   getNumParams()  const    override { return kNumParams; }

    std::vector<PortInfo> getPorts() const override;
    std::string           getName()  const override { return "EnvFollow"; }

private:
    double sampleRate_ = 96000.0;
    float  env_        = 0.0f;   // aktualny poziom obwiedni
    float  rmsAcc_     = 0.0f;   // akumulator RMS
    int    rmsCount_   = 0;

    static constexpr int kRmsWindow = 512;  // ~5ms @ 96kHz

    float params_[kNumParams] = { 0.1f, 0.3f, 0.0f, 0.5f };
};

} // namespace synth
