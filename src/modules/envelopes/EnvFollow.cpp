#include "EnvFollow.h"
#include <algorithm>
#include <cmath>

namespace synth {

void EnvFollow::prepare(double sampleRate, int /*blockSize*/) {
    sampleRate_ = sampleRate;
    env_        = 0.0f;
    rmsAcc_     = 0.0f;
    rmsCount_   = 0;
}

std::vector<PortInfo> EnvFollow::getPorts() const {
    return {
        { "Audio In", SignalType::Audio, false },
        { "Env CV",   SignalType::CV,    true  },
    };
}

void EnvFollow::process(const float* const* inputs,
                        float* const*       outputs,
                        int                 numSamples) {
    const float* in  = inputs  ? inputs[0]  : nullptr;
    float*       out = outputs[0];

    // params → ms → współczynniki one-pole
    float attMs  = 0.1f + params_[kAttack]  * 499.9f;  // 0.1..500ms
    float relMs  = 10.0f + params_[kRelease] * 4990.0f; // 10..5000ms
    float gain   = std::pow(10.0f, params_[kGain] * 24.0f / 20.0f); // 0..+24dB
    bool  isPeak = params_[kMode] > 0.5f;

    float sr    = static_cast<float>(sampleRate_);
    float attC  = std::exp(-1.0f / (sr * attMs  * 0.001f));
    float relC  = std::exp(-1.0f / (sr * relMs  * 0.001f));

    for (int i = 0; i < numSamples; ++i) {
        float x = in ? std::fabs(in[i] * gain) : 0.0f;

        if (isPeak) {
            // Peak follower
            if (x > env_)
                env_ = attC * env_ + (1.0f - attC) * x;
            else
                env_ = relC * env_;
        } else {
            // RMS follower: akumuluj x²
            rmsAcc_ += x * x;
            ++rmsCount_;
            if (rmsCount_ >= kRmsWindow) {
                float rms = std::sqrt(rmsAcc_ / static_cast<float>(kRmsWindow));
                rmsAcc_   = 0.0f;
                rmsCount_ = 0;
                if (rms > env_)
                    env_ = attC * env_ + (1.0f - attC) * rms;
                else
                    env_ = relC * env_;
            }
        }

        out[i] = std::clamp(env_, 0.0f, 1.0f);
    }
}

void EnvFollow::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
