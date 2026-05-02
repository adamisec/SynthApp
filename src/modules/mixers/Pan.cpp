#include "Pan.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace synth {

std::vector<PortInfo> Pan::getPorts() const {
    return {
        { "Audio In", SignalType::Audio, false },
        { "Pan CV",   SignalType::CV,    false },
        { "Left Out", SignalType::Audio, true  },
        { "Right Out",SignalType::Audio, true  },
    };
}

void Pan::process(const float* const* inputs,
                  float* const*       outputs,
                  int                 numSamples) {
    const float* audioIn = inputs[0];
    const float* panCV   = inputs[1];
    float* left  = outputs[0];
    float* right = outputs[1];

    float basePan = params_[kPan];

    for (int i = 0; i < numSamples; ++i) {
        float pan = basePan;
        if (panCV) pan = std::clamp(pan + panCV[i] * 0.5f, 0.0f, 1.0f);

        // Equal-power: angle 0..π/2
        float angle = pan * std::numbers::pi_v<float> * 0.5f;
        float gainL = std::cos(angle);
        float gainR = std::sin(angle);

        float audio = audioIn ? audioIn[i] : 0.0f;
        left[i]  = audio * gainL;
        right[i] = audio * gainR;
    }
}

void Pan::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
