#include "ModWheel.h"
#include <algorithm>

namespace synth {

std::vector<PortInfo> ModWheel::getPorts() const {
    return {
        { "CV Out", SignalType::CV, true },
    };
}

void ModWheel::process(const float* const* /*inputs*/,
                       float* const*       outputs,
                       int                 numSamples) {
    float val = params_[kValue];
    for (int i = 0; i < numSamples; ++i)
        outputs[0][i] = val;
}

void ModWheel::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
