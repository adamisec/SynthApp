#include "FxReverb.h"
#include <algorithm>
#include <numeric>

namespace synth {

void FxReverb::prepare(double sampleRate, int /*blockSize*/) {
    sampleRate_ = sampleRate;
    double scale = sampleRate / 44100.0;

    for (int i = 0; i < kNumCombs; ++i) {
        combL_[i].resize(static_cast<int>(kCombTuning[i] * scale));
        combR_[i].resize(static_cast<int>((kCombTuning[i] + kStereoSpread) * scale));
    }
    for (int i = 0; i < kNumAllpasses; ++i) {
        apL_[i].resize(static_cast<int>(kAllpassTuning[i] * scale));
        apR_[i].resize(static_cast<int>((kAllpassTuning[i] + kStereoSpread) * scale));
        apL_[i].feedback = apR_[i].feedback = 0.5f;
    }
    reset();

    // Ustaw parametry
    setParam(kRoomSize, params_[kRoomSize]);
    setParam(kDamp,     params_[kDamp]);
}

void FxReverb::reset() {
    for (auto& c : combL_) std::fill(c.buf.begin(), c.buf.end(), 0.0f);
    for (auto& c : combR_) std::fill(c.buf.begin(), c.buf.end(), 0.0f);
    for (auto& a : apL_)   std::fill(a.buf.begin(), a.buf.end(), 0.0f);
    for (auto& a : apR_)   std::fill(a.buf.begin(), a.buf.end(), 0.0f);
    for (auto& c : combL_) c.store = 0.0f;
    for (auto& c : combR_) c.store = 0.0f;
}

std::vector<PortInfo> FxReverb::getPorts() const {
    return {
        { "Audio In", SignalType::Audio, false }, // in 0
        { "Out L",    SignalType::Audio, true  }, // out 0
        { "Out R",    SignalType::Audio, true  }, // out 1
    };
}

void FxReverb::process(const float* const* inputs,
                       float* const*       outputs,
                       int                 numSamples) {
    const float* in  = inputs[0];
    float*       outL = outputs[0];
    float*       outR = outputs[1];

    if (!in) {
        std::fill(outL, outL + numSamples, 0.0f);
        std::fill(outR, outR + numSamples, 0.0f);
        return;
    }

    float mix    = params_[kMix];
    float dry    = 1.0f - mix;
    float wet    = mix;
    float width  = params_[kWidth];
    float wet1   = wet * (width * 0.5f + 0.5f);
    float wet2   = wet * ((1.0f - width) * 0.5f);

    for (int i = 0; i < numSamples; ++i) {
        float x = in[i] * 0.015f; // scale input (Freeverb standard)

        float sumL = 0.0f, sumR = 0.0f;
        for (int c = 0; c < kNumCombs; ++c) {
            sumL += combL_[c].tick(x);
            sumR += combR_[c].tick(x);
        }
        for (int a = 0; a < kNumAllpasses; ++a) {
            sumL = apL_[a].tick(sumL);
            sumR = apR_[a].tick(sumR);
        }

        outL[i] = dry * in[i] + wet1 * sumL + wet2 * sumR;
        outR[i] = dry * in[i] + wet1 * sumR + wet2 * sumL;
    }
}

void FxReverb::setParam(int i, float v) {
    if (i < 0 || i >= kNumParams) return;
    params_[i] = std::clamp(v, 0.0f, 1.0f);

    if (i == kRoomSize) {
        float feedback = 0.7f + params_[kRoomSize] * 0.28f; // 0.70..0.98
        for (auto& c : combL_) c.feedback = feedback;
        for (auto& c : combR_) c.feedback = feedback;
    }
    if (i == kDamp) {
        for (auto& c : combL_) c.setDamp(params_[kDamp]);
        for (auto& c : combR_) c.setDamp(params_[kDamp]);
    }
}

} // namespace synth
