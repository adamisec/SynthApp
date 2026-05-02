#include "FltNord.h"
#include <algorithm>
#include <numbers>

namespace synth {

void FltNord::prepare(double sampleRate, int /*blockSize*/) {
    sampleRate_ = sampleRate;
    reset();
}

void FltNord::reset() {
    ic1eq_ = ic2eq_ = 0.0f;
}

std::vector<PortInfo> FltNord::getPorts() const {
    return {
        { "Audio In",   SignalType::Audio, false },
        { "Cutoff CV",  SignalType::CV,    false },
        { "Res CV",     SignalType::CV,    false },
        { "LP Out",     SignalType::Audio, true  },
        { "HP Out",     SignalType::Audio, true  },
        { "BP Out",     SignalType::Audio, true  },
        { "Notch Out",  SignalType::Audio, true  },
    };
}

// Cytonik SVF — Andrew Simper (2011)
// Dokładny ZDF state-variable, brak pre-warping błędu
void FltNord::process(const float* const* inputs,
                      float* const*       outputs,
                      int                 numSamples) {
    const float* audioIn  = inputs[0];
    const float* cutoffCV = inputs[1];
    const float* resCV    = inputs[2];

    float* lp = outputs[0];
    float* hp = outputs[1];
    float* bp = outputs[2];
    float* nt = outputs[3];

    float baseCutoff = params_[kCutoff];
    float baseRes    = params_[kResonance];

    for (int i = 0; i < numSamples; ++i) {
        float fc = baseCutoff;
        if (cutoffCV) fc = std::clamp(fc + cutoffCV[i] * 0.2f, 0.0f, 1.0f);
        float res = baseRes;
        if (resCV) res = std::clamp(res + resCV[i] * 0.5f, 0.0f, 1.0f);

        float cutHz = 20.0f * std::pow(1000.0f, fc);
        cutHz = std::clamp(cutHz, 20.0f, (float)(sampleRate_ * 0.49));

        float g = std::tan(std::numbers::pi_v<float> * cutHz / (float)sampleRate_);
        float k = 2.0f - 1.99f * res;  // Q: 0.5..2 (unipolarna stabilność)

        float a1 = 1.0f / (1.0f + g * (g + k));
        float a2 = g * a1;
        float a3 = g * a2;

        float v0 = audioIn ? audioIn[i] : 0.0f;
        float v3 = v0 - ic2eq_;
        float v1 = a1 * ic1eq_ + a2 * v3;
        float v2 = ic2eq_ + a2 * ic1eq_ + a3 * v3;

        ic1eq_ = 2.0f * v1 - ic1eq_;
        ic2eq_ = 2.0f * v2 - ic2eq_;

        lp[i] = v2;
        bp[i] = v1;
        hp[i] = v0 - k * v1 - v2;
        nt[i] = v0 - k * v1;
    }
}

void FltNord::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
