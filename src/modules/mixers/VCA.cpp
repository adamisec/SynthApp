#include "VCA.h"
#include <algorithm>
#include <cmath>

namespace synth {

std::vector<PortInfo> VCA::getPorts() const {
    return {
        { "Audio In", SignalType::Audio, false },
        { "CV Gain",  SignalType::CV,    false },
        { "Audio Out",SignalType::Audio, true  },
    };
}

void VCA::process(const float* const* inputs,
                  float* const*       outputs,
                  int                 numSamples) {
    const float* audioIn = inputs[0];
    const float* cvIn    = inputs[1];
    float*       out     = outputs[0];

    float gain = params_[kGain];
    bool  expo = params_[kMode] > 0.5f;

    for (int i = 0; i < numSamples; ++i) {
        float audio = audioIn ? audioIn[i] : 0.0f;
        float cv    = cvIn    ? std::clamp(cvIn[i], 0.0f, 1.0f) : 1.0f;

        // Tryb wykładniczy: -∞ dB przy cv=0, 0 dB przy cv=1
        float g = expo ? cv * cv : cv;  // cv² ≈ -6dB/dekadę (wystarczające)
        g *= gain;

        out[i] = audio * g;
    }
}

void VCA::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
