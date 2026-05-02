#pragma once
#include "engine/Module.h"
#include <array>
#include <vector>
#include <cmath>

namespace synth {

// ── FxChorus — chórus wielogłosowy (3 głosy) ─────────────────────────────────
// Każdy głos: linia opóźnienia 1..30ms + LFO sinus z przesunięciem fazowym 120°
// Wyjście L/R sumuje głosy z różnymi fazami LFO dla efektu stereo
//
// Wejście:  [0] Audio In
// Wyjście:  [0] Out L, [1] Out R
class FxChorus : public Module {
public:
    enum Param {
        kRate,   // [0..1] → LFO 0.1..5 Hz
        kDepth,  // [0..1] → głębokość modulacji (ms) 0..15ms
        kDelay,  // [0..1] → czas bazowy 1..30ms
        kMix,    // [0..1] → drywet
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
    std::string           getName()  const override { return "FxChorus"; }

private:
    float readInterp(const std::vector<float>& buf, int writePos, float delayInSamples) const noexcept;

    static constexpr int   kNumVoices  = 3;
    static constexpr float kMaxDelayMs = 35.0f;

    double sampleRate_ = 96000.0;

    std::vector<float>         buf_;
    int                        writePos_ = 0;
    std::array<float, kNumVoices> lfoPhase_;

    float params_[kNumParams] = { 0.2f, 0.5f, 0.3f, 0.7f };
};

} // namespace synth
