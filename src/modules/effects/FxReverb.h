#pragma once
#include "engine/Module.h"
#include <array>
#include <vector>
#include <cmath>

namespace synth {

// ── FxReverb — Freeverb (Schroeder 1962 / Moorer 1979) ──────────────────────
// 8 równoległych comb IIR + 4 szeregowe allpass, stereo
// Wejście: [0] Audio In (mono)
// Wyjście: [0] L, [1] R
class FxReverb : public Module {
public:
    enum Param {
        kRoomSize, // [0..1] → feedback komb 0.7..0.98
        kDamp,     // [0..1] → tłumienie HF w kombach 0..1
        kWidth,    // [0..1] → stereo spread 0..1
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
    std::string           getName()  const override { return "FxReverb"; }

private:
    // Comb filter z tłumieniem HF (Freeverb LPF inside loop)
    struct CombFilter {
        std::vector<float> buf;
        int   writePos = 0;
        float feedback = 0.84f;
        float damp1    = 0.2f;  // = damp param
        float damp2    = 0.8f;  // = 1 - damp
        float store    = 0.0f;  // LPF state

        void resize(int n) { buf.assign(n, 0.0f); writePos = 0; }
        void setDamp(float d) { damp1 = d; damp2 = 1.0f - d; }

        float tick(float x) noexcept {
            float y = buf[writePos];
            store   = y * damp2 + store * damp1;
            buf[writePos] = x + store * feedback;
            if (++writePos >= static_cast<int>(buf.size())) writePos = 0;
            return y;
        }
    };

    // Allpass (brak LPF)
    struct AllpassFilter {
        std::vector<float> buf;
        int   writePos = 0;
        float feedback = 0.5f;

        void resize(int n) { buf.assign(n, 0.0f); writePos = 0; }

        float tick(float x) noexcept {
            float bufout = buf[writePos];
            float y      = -x + bufout;
            buf[writePos] = x + bufout * feedback;
            if (++writePos >= static_cast<int>(buf.size())) writePos = 0;
            return y;
        }
    };

    // Freeverb: 8 combów + 4 allpassów na kanał
    static constexpr int kNumCombs    = 8;
    static constexpr int kNumAllpasses = 4;

    // Tuning Freeverb (przy 44.1kHz, skalowane do bieżącego SR)
    static constexpr int kCombTuning[kNumCombs]     = {1116,1188,1277,1356,1422,1491,1557,1617};
    static constexpr int kAllpassTuning[kNumAllpasses] = {556, 441, 341, 225};
    static constexpr int kStereoSpread = 23;

    std::array<CombFilter,    kNumCombs>     combL_, combR_;
    std::array<AllpassFilter, kNumAllpasses> apL_,   apR_;

    double sampleRate_ = 96000.0;
    float  params_[kNumParams] = { 0.5f, 0.5f, 1.0f, 0.5f };
};

} // namespace synth
