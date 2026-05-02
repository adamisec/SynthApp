#include "LfoA.h"
#include <algorithm>
#include <numbers>

namespace synth {

void LfoA::prepare(double sampleRate, int /*blockSize*/) {
    sampleRate_ = sampleRate;
}

std::vector<PortInfo> LfoA::getPorts() const {
    return {
        { "Sync Gate", SignalType::Gate, false },  // in 0 (opcjonalny)
        { "CV Out",    SignalType::CV,   true  },  // out 0
    };
}

void LfoA::process(const float* const* inputs,
                   float* const*       outputs,
                   int                 numSamples) {
    const float* syncIn = inputs[0];
    float* out = outputs[0];

    float rateHz = 0.05f * std::pow(400.0f, params_[kRate]);  // 0.05..20 Hz
    float dt     = rateHz / static_cast<float>(sampleRate_);
    float depth  = params_[kDepth];
    int   wave   = static_cast<int>(params_[kWaveform] * 4.99f);

    for (int i = 0; i < numSamples; ++i) {
        // Sync: zbocze narastające resety fazę
        if (syncIn) {
            bool sync = syncIn[i] > 0.5f;
            if (sync && !prevSync_) phase_ = 0.0f;
            prevSync_ = sync;
        }

        float val = 0.0f;
        switch (wave) {
            case 0: val = std::sin(phase_ * 2.0f * std::numbers::pi_v<float>); break;
            case 1: val = (phase_ < 0.5f) ? (4.0f*phase_-1.0f) : (3.0f-4.0f*phase_); break;
            case 2: val = 2.0f * phase_ - 1.0f; break;
            case 3: val = (phase_ < 0.5f) ? 1.0f : -1.0f; break;
            case 4:  // Sample & Hold — zmienia wartość tylko przy przejściu 0→
                if (phase_ < dt) sampleHold_ = (float)rand() / RAND_MAX * 2.0f - 1.0f;
                val = sampleHold_;
                break;
        }

        out[i] = val * depth;

        phase_ += dt;
        if (phase_ >= 1.0f) phase_ -= 1.0f;
    }
}

void LfoA::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
