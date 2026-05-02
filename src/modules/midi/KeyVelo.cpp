#include "KeyVelo.h"
#include <algorithm>

namespace synth {

std::vector<PortInfo> KeyVelo::getPorts() const {
    return {
        { "Velocity CV",  SignalType::CV, true },  // wyjście 0
        { "Inv Velocity", SignalType::CV, true },  // wyjście 1
    };
}

void KeyVelo::process(const float* const* /*inputs*/,
                      float* const*       outputs,
                      int                 numSamples) {
    float* velOut = outputs[0];
    float* invOut = outputs[1];

    float v = params_[kVelocity];
    float inv = 1.0f - v;

    for (int i = 0; i < numSamples; ++i) {
        velOut[i] = v;
        invOut[i] = inv;
    }
}

void KeyVelo::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
