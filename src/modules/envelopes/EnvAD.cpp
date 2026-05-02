#include "EnvAD.h"
#include <algorithm>
#include <cmath>

namespace synth {

float EnvAD::timeToCoeff(float param) const noexcept {
    float ms      = kMinTimeMs * std::pow(kMaxTimeMs / kMinTimeMs, param);
    float samples = ms * static_cast<float>(sampleRate_) / 1000.0f;
    return std::exp(-1.0f / std::max(samples, 1.0f));
}

void EnvAD::prepare(double sampleRate, int /*blockSize*/) {
    sampleRate_ = sampleRate;
    reset();
}

void EnvAD::reset() {
    stage_     = Stage::Idle;
    value_     = 0.0f;
    prevGate_  = false;
    triggered_ = false;
}

std::vector<PortInfo> EnvAD::getPorts() const {
    return {
        { "Trigger",     SignalType::Gate, false },  // in 0
        { "Env Out",     SignalType::CV,   true  },  // out 0
        { "Inv Env Out", SignalType::CV,   true  },  // out 1
    };
}

void EnvAD::process(const float* const* inputs,
                    float* const*       outputs,
                    int                 numSamples) {
    const float* trigIn = inputs[0];
    float* envOut = outputs[0];
    float* invOut = outputs[1];

    float attackCoeff = timeToCoeff(params_[kAttack]);
    float decayCoeff  = timeToCoeff(params_[kDecay]);
    bool  loop        = params_[kLoop] >= 0.5f;

    for (int i = 0; i < numSamples; ++i) {
        // Detekcja zbocza wyzwalającego
        bool gate = triggered_;
        if (trigIn) gate = trigIn[i] > 0.5f;

        if (gate && !prevGate_) {
            stage_ = Stage::Attack;
            triggered_ = false;
        }
        prevGate_ = gate;

        switch (stage_) {
            case Stage::Idle:
                value_ = 0.0f;
                break;

            case Stage::Attack:
                value_ += attackCoeff * (1.02f - value_);
                if (value_ >= 1.0f) {
                    value_ = 1.0f;
                    stage_ = Stage::Decay;
                }
                break;

            case Stage::Decay:
                value_ += decayCoeff * (0.0f - value_);
                if (value_ < 0.0001f) {
                    value_ = 0.0f;
                    // Tryb loop: restart bez czekania na trigger
                    stage_ = loop ? Stage::Attack : Stage::Idle;
                }
                break;
        }

        envOut[i] = value_;
        invOut[i] = 1.0f - value_;
    }
}

void EnvAD::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
