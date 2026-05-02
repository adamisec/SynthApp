#include "Out.h"
#include <algorithm>

namespace synth {

std::vector<PortInfo> Out::getPorts() const {
    return {
        { "In L",  SignalType::Audio, false },  // wejście 0
        { "In R",  SignalType::Audio, false },  // wejście 1 (opcjonalne)
        { "Out L", SignalType::Audio, true  },  // wyjście 0
        { "Out R", SignalType::Audio, true  },  // wyjście 1
    };
}

void Out::process(const float* const* inputs,
                  float* const*       outputs,
                  int                 numSamples) {
    const float* inL = inputs[0];
    const float* inR = inputs[1];

    float* outL = outputs[0];
    float* outR = outputs[1];

    float level = params_[kLevel];

    if (inL && inR) {
        // Stereo
        for (int i = 0; i < numSamples; ++i) {
            outL[i] = inL[i] * level;
            outR[i] = inR[i] * level;
        }
    } else if (inL) {
        // Mono → stereo
        for (int i = 0; i < numSamples; ++i) {
            outL[i] = inL[i] * level;
            outR[i] = inL[i] * level;
        }
    } else {
        // Cisza
        for (int i = 0; i < numSamples; ++i)
            outL[i] = outR[i] = 0.0f;
    }
}

void Out::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
