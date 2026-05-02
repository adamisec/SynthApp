#pragma once
#include "engine/Module.h"
#include <array>
#include <cmath>

namespace synth {

// ── FltPhase — phaser allpass (2/4/6/8 sekcji) ───────────────────────────────
// Każda sekcja: H(z) = (a - z^{-1}) / (1 - a * z^{-1})
// gdzie a = (tan(π*fc/fs) - 1) / (tan(π*fc/fs) + 1)
// Feedback dla rezonansu (notch depth).
//
// Wejścia: [0] Audio In, [1] Rate CV (modulacja LFO na zewnątrz)
// Wyjścia: [0] Audio Out (wet=100%), [1] Direct Out (drywet)
class FltPhase : public Module {
public:
    static constexpr int kMaxStages = 8;

    enum Param {
        kFreq,     // [0..1] → centrum phasera 20Hz..8kHz (log)
        kDepth,    // [0..1] → zakres modulacji LFO (w oktawach, 0..4)
        kRate,     // [0..1] → szybkość LFO 0.01..10 Hz (log)
        kFeedback, // [0..1] → głębokość notchy (-1..+1 feedback)
        kStages,   // [0..1] → 2/4/6/8 sekcji (kwantyzowane)
        kMix,      // [0..1] → drywet
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
    std::string           getName()  const override { return "FltPhase"; }

private:
    // Stan jednej sekcji allpass
    struct AllpassStage {
        float z1 = 0.0f;
        float tick(float x, float a) noexcept {
            float y = a * x + z1;
            z1 = x - a * y;
            return y;
        }
    };

    double sampleRate_  = 96000.0;
    float  lfoPhase_    = 0.0f;
    float  feedback_z_  = 0.0f;  // stan feedbacku

    std::array<AllpassStage, kMaxStages> stages_;

    float params_[kNumParams] = { 0.3f, 0.5f, 0.2f, 0.5f, 0.5f, 0.5f };
    //                             ~200Hz  2oct  ~0.5Hz  50%fb  4stages  50%mix
};

} // namespace synth
