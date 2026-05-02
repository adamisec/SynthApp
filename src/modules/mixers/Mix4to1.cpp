#include "Mix4to1.h"
#include <algorithm>

namespace synth {

std::vector<PortInfo> Mix4to1::getPorts() const {
    return {
        { "In A",   SignalType::Audio, false },
        { "In B",   SignalType::Audio, false },
        { "In C",   SignalType::Audio, false },
        { "In D",   SignalType::Audio, false },
        { "Out",    SignalType::Audio, true  },
    };
}

void Mix4to1::process(const float* const* inputs,
                      float* const*       outputs,
                      int                 numSamples) {
    float* out = outputs[0];
    float levels[4] = { params_[kLevelA], params_[kLevelB],
                        params_[kLevelC], params_[kLevelD] };

    for (int i = 0; i < numSamples; ++i) {
        float sum = 0.0f;
        for (int ch = 0; ch < 4; ++ch)
            if (inputs[ch]) sum += inputs[ch][i] * levels[ch];
        out[i] = sum;
    }
}

void Mix4to1::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
