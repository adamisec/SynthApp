#pragma once
#include "engine/Module.h"
#include <cmath>

namespace synth {

// ── FxDistort — waveshaper (4 algorytmy) ────────────────────────────────────
// 0 = Soft Clip  : tanh(drive * x)
// 1 = Hard Clip  : clamp(x * drive, -1, +1)
// 2 = Foldback   : fałdowanie sygnału przekraczającego próg
// 3 = Bit Crush  : kwantyzacja do N bitów (8..2)
//
// Wejście: [0] Audio In, [1] Drive CV
// Wyjście: [0] Audio Out
class FxDistort : public Module {
public:
    enum Param {
        kDrive,     // [0..1] → wzmocnienie wejścia 1..40x (log)
        kType,      // [0..1] → 4 typy: 0=soft, 1=hard, 2=fold, 3=bitcrush
        kOutput,    // [0..1] → gain wyjściowy (kompensacja głośności)
        kMix,       // [0..1] → drywet
        kNumParams
    };

    void prepare(double sampleRate, int blockSize) override;
    void reset()   override {}

    void  process(const float* const* inputs,
                  float* const*       outputs,
                  int                 numSamples) override;

    void  setParam(int i, float v) override;
    float getParam(int i) const    override { return params_[i]; }
    int   getNumParams()  const    override { return kNumParams; }

    std::vector<PortInfo> getPorts() const override;
    std::string           getName()  const override { return "FxDistort"; }

private:
    static float softClip(float x, float drive) noexcept {
        return std::tanh(x * drive);
    }
    static float hardClip(float x, float drive) noexcept {
        float y = x * drive;
        if (y >  1.0f) return  1.0f;
        if (y < -1.0f) return -1.0f;
        return y;
    }
    static float foldback(float x, float drive) noexcept {
        float y = x * drive;
        // Wielokrotne fałdowanie
        while (y >  1.0f) y =  2.0f - y;
        while (y < -1.0f) y = -2.0f - y;
        return y;
    }
    static float bitCrush(float x, float drive) noexcept {
        // drive [0..1] → bits 8..2
        int bits = 8 - static_cast<int>(drive * 6.0f);
        bits = (bits < 2) ? 2 : bits;
        float levels = static_cast<float>(1 << (bits - 1));
        return std::round(x * levels) / levels;
    }

    double sampleRate_ = 96000.0;
    float  params_[kNumParams] = { 0.3f, 0.0f, 0.5f, 1.0f };
    //                              low   soft    mid   full
};

} // namespace synth
