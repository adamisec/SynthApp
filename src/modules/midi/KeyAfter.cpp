#include "KeyAfter.h"
#include <algorithm>

namespace synth {

std::vector<PortInfo> KeyAfter::getPorts() const {
    return {
        { "After CV",     SignalType::CV, true },  // wyjście 0
        { "Inv After CV", SignalType::CV, true },  // wyjście 1
    };
}

void KeyAfter::process(const float* const* /*inputs*/,
                       float* const*       outputs,
                       int                 numSamples) {
    float  v   = params_[kAftertouch];
    float  inv = 1.0f - v;
    float* out0 = outputs[0];
    float* out1 = outputs[1];

    for (int i = 0; i < numSamples; ++i) {
        out0[i] = v;
        out1[i] = inv;
    }
}

void KeyAfter::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
