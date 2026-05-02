#include "Mix8to1.h"
#include <algorithm>

namespace synth {

std::vector<PortInfo> Mix8to1::getPorts() const {
    return {
        { "In 1",  SignalType::Audio, false },
        { "In 2",  SignalType::Audio, false },
        { "In 3",  SignalType::Audio, false },
        { "In 4",  SignalType::Audio, false },
        { "In 5",  SignalType::Audio, false },
        { "In 6",  SignalType::Audio, false },
        { "In 7",  SignalType::Audio, false },
        { "In 8",  SignalType::Audio, false },
        { "Out",   SignalType::Audio, true  },
    };
}

void Mix8to1::process(const float* const* inputs,
                      float* const*       outputs,
                      int                 numSamples) {
    float* out = outputs[0];

    for (int i = 0; i < numSamples; ++i) {
        float sum = 0.0f;
        for (int ch = 0; ch < 8; ++ch) {
            if (inputs && inputs[ch])
                sum += inputs[ch][i] * params_[kLevel0 + ch];
        }
        out[i] = sum;
    }
}

void Mix8to1::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
