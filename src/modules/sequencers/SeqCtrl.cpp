#include "SeqCtrl.h"
#include <algorithm>
#include <cmath>

namespace synth {

SeqCtrl::SeqCtrl() {
    // Domyślne wartości: sinusoidalna krzywa CV
    for (int s = 0; s < kSteps; ++s) {
        float angle = static_cast<float>(s) / kSteps * 2.0f * 3.14159265f;
        params_[kStep0 + s] = (std::sin(angle) + 1.0f) * 0.5f;
    }
    params_[kLength] = (8.0f - 1.0f) / 15.0f;
    params_[kSmooth] = 0.0f;
}

void SeqCtrl::prepare(double sampleRate, int /*blockSize*/) {
    sampleRate_ = sampleRate;
}

void SeqCtrl::reset() {
    step_        = 0;
    prevClock_   = false;
    prevReset_   = false;
    gateCounter_ = 0;
    currentCV_   = 0.0f;
}

std::vector<PortInfo> SeqCtrl::getPorts() const {
    return {
        { "Clock In",  SignalType::Gate, false },
        { "Reset In",  SignalType::Gate, false },
        { "CV Out",    SignalType::CV,   true  },
        { "Gate Out",  SignalType::Gate, true  },
    };
}

void SeqCtrl::process(const float* const* inputs,
                      float* const*       outputs,
                      int                 numSamples) {
    const float* clkIn   = inputs  ? inputs[0] : nullptr;
    const float* rstIn   = inputs  ? inputs[1] : nullptr;
    float*       cvOut   = outputs[0];
    float*       gateOut = outputs[1];

    int length = std::max(1, static_cast<int>(params_[kLength] * 15.99f) + 1);

    // Wygładzanie: [0..1] → 0..200ms
    float smoothMs   = params_[kSmooth] * 200.0f;
    float smoothCoef = (smoothMs > 0.0f)
                       ? std::exp(-1.0f / (static_cast<float>(sampleRate_) * smoothMs * 0.001f))
                       : 0.0f;

    // Cel CV: params bipolarny [0..1] → [-1..1]
    float targetCV = params_[kStep0 + step_] * 2.0f - 1.0f;

    for (int i = 0; i < numSamples; ++i) {
        // Reset (rising edge)
        bool rst = rstIn && rstIn[i] > 0.5f;
        if (rst && !prevReset_) {
            step_        = 0;
            gateCounter_ = 0;
            targetCV     = params_[kStep0 + step_] * 2.0f - 1.0f;
        }
        prevReset_ = rst;

        // Clock (rising edge)
        bool clk = clkIn && clkIn[i] > 0.5f;
        if (clk && !prevClock_) {
            step_        = (step_ + 1) % length;
            gateCounter_ = kGateLength;
            targetCV     = params_[kStep0 + step_] * 2.0f - 1.0f;
        }
        prevClock_ = clk;

        // Wygładzanie
        if (smoothCoef > 0.0f)
            currentCV_ = smoothCoef * currentCV_ + (1.0f - smoothCoef) * targetCV;
        else
            currentCV_ = targetCV;

        cvOut[i]   = currentCV_;
        gateOut[i] = (gateCounter_ > 0) ? 1.0f : 0.0f;
        if (gateCounter_ > 0) --gateCounter_;
    }
}

void SeqCtrl::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
