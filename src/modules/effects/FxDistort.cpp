#include "FxDistort.h"
#include <algorithm>
#include <cmath>

namespace synth {

void FxDistort::prepare(double sampleRate, int /*blockSize*/) {
    sampleRate_ = sampleRate;
}

std::vector<PortInfo> FxDistort::getPorts() const {
    return {
        { "Audio In", SignalType::Audio, false },
        { "Drive CV", SignalType::CV,    false },
        { "Audio Out",SignalType::Audio, true  },
    };
}

void FxDistort::process(const float* const* inputs,
                        float* const*       outputs,
                        int                 numSamples) {
    const float* in      = inputs[0];
    const float* driveCV = inputs[1];
    float*       out     = outputs[0];

    if (!in) { std::fill(out, out + numSamples, 0.0f); return; }

    // drive: log 1..40
    float driveBase = std::pow(40.0f, params_[kDrive]);
    float outputGain = params_[kOutput] * 2.0f;   // 0..2
    float mix        = params_[kMix];
    float dry        = 1.0f - mix;

    int type = static_cast<int>(params_[kType] * 3.99f);
    type = std::clamp(type, 0, 3);

    for (int i = 0; i < numSamples; ++i) {
        float drive = driveBase;
        if (driveCV) drive *= std::exp2(driveCV[i]);
        drive = std::clamp(drive, 1.0f, 80.0f);

        float y;
        switch (type) {
            case 0:  y = softClip (in[i], drive); break;
            case 1:  y = hardClip (in[i], drive); break;
            case 2:  y = foldback (in[i], drive); break;
            default: y = bitCrush (in[i], params_[kDrive]); break;
        }

        out[i] = (dry * in[i] + mix * y) * outputGain;
    }
}

void FxDistort::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
