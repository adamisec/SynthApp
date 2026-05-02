#include "LfoB.h"
#include <algorithm>
#include <numbers>
#include <cstdlib>

namespace synth {

void LfoB::prepare(double sampleRate, int /*blockSize*/) {
    sampleRate_ = sampleRate;
}

void LfoB::reset() {
    phase_      = params_[kStartPhase];
    sampleHold_ = 0.0f;
    prevSync_   = false;
}

std::vector<PortInfo> LfoB::getPorts() const {
    return {
        { "Sync In",  SignalType::Gate, false },  // in 0
        { "Rate CV",  SignalType::CV,   false },  // in 1
        { "CV Out",   SignalType::CV,   true  },  // out 0
    };
}

void LfoB::process(const float* const* inputs,
                   float* const*       outputs,
                   int                 numSamples) {
    const float* syncIn   = inputs ? inputs[0] : nullptr;
    const float* rateCVIn = inputs ? inputs[1] : nullptr;
    float*       out      = outputs[0];

    float baseRate = 0.05f * std::pow(400.0f, params_[kRate]);
    float depth    = params_[kDepth];
    int   wave     = static_cast<int>(params_[kWaveform] * 4.99f);

    for (int i = 0; i < numSamples; ++i) {
        // Sync: rising edge → reset do kStartPhase
        if (syncIn) {
            bool sync = syncIn[i] > 0.5f;
            if (sync && !prevSync_) phase_ = params_[kStartPhase];
            prevSync_ = sync;
        }

        // Rate CV: 1V/oct modulacja tempa
        float rate = baseRate;
        if (rateCVIn) rate *= std::pow(2.0f, rateCVIn[i]);
        float dt = std::clamp(rate / static_cast<float>(sampleRate_), 0.0f, 0.5f);

        float val = 0.0f;
        switch (wave) {
            case 0: val = std::sin(phase_ * 2.0f * std::numbers::pi_v<float>); break;
            case 1: val = (phase_ < 0.5f) ? (4.0f*phase_-1.0f) : (3.0f-4.0f*phase_); break;
            case 2: val = 2.0f * phase_ - 1.0f; break;
            case 3: val = (phase_ < 0.5f) ? 1.0f : -1.0f; break;
            case 4:
                if (phase_ < dt) sampleHold_ = static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f;
                val = sampleHold_;
                break;
        }

        out[i] = val * depth;

        phase_ += dt;
        if (phase_ >= 1.0f) phase_ -= 1.0f;
    }
}

void LfoB::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
