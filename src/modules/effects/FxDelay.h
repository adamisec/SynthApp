#pragma once
#include "engine/Module.h"
#include <vector>
#include <cmath>

namespace synth {

// ── FxDelay — linia opóźniająca z interpolacją liniową ──────────────────────
// Czas 1ms..2000ms (lub synchronizacja do tempa: 1/32..1/1)
// Stereo ping-pong optional (feedback skrośny L→R→L)
//
// Wejście:  [0] In L, [1] In R (opcja), [2] Time CV (1V/oct → x2 per oct)
// Wyjście:  [0] Out L, [1] Out R
class FxDelay : public Module {
public:
    enum Param {
        kTime,      // [0..1] → 1ms..2000ms (log)
        kFeedback,  // [0..1] → 0..0.95
        kMix,       // [0..1] → drywet
        kPingPong,  // <0.5 = normal, ≥0.5 = ping-pong
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
    std::string           getName()  const override { return "FxDelay"; }

private:
    // Liniowa interpolacja z bufora kołowego
    float readInterp(const std::vector<float>& buf, int writePos, float delayInSamples) const noexcept;

    static constexpr double kMaxDelayMs = 2000.0;

    double sampleRate_ = 96000.0;

    std::vector<float> bufL_, bufR_;
    int   writePosL_ = 0;
    int   writePosR_ = 0;

    float params_[kNumParams] = { 0.4f, 0.4f, 0.5f, 0.0f };
    //                             ~250ms   40%   50%   normal
};

} // namespace synth
