#include "FxChorus.h"
#include <algorithm>
#include <numbers>

namespace synth {

void FxChorus::prepare(double sampleRate, int /*blockSize*/) {
    sampleRate_ = sampleRate;
    int maxSamples = static_cast<int>(kMaxDelayMs * 0.001f * sampleRate) + 2;
    buf_.assign(maxSamples, 0.0f);
    writePos_ = 0;
    lfoPhase_.fill(0.0f);
    // Rozłóż fazy 3 głosów o 120°
    lfoPhase_[0] = 0.0f;
    lfoPhase_[1] = 1.0f / 3.0f;
    lfoPhase_[2] = 2.0f / 3.0f;
}

void FxChorus::reset() {
    std::fill(buf_.begin(), buf_.end(), 0.0f);
    writePos_ = 0;
    lfoPhase_[0] = 0.0f;
    lfoPhase_[1] = 1.0f / 3.0f;
    lfoPhase_[2] = 2.0f / 3.0f;
}

std::vector<PortInfo> FxChorus::getPorts() const {
    return {
        { "Audio In", SignalType::Audio, false },
        { "Out L",    SignalType::Audio, true  },
        { "Out R",    SignalType::Audio, true  },
    };
}

float FxChorus::readInterp(const std::vector<float>& buf, int writePos, float delayInSamples) const noexcept {
    int   n    = static_cast<int>(buf.size());
    int   iDel = static_cast<int>(delayInSamples);
    float frac = delayInSamples - static_cast<float>(iDel);
    int   p0   = (writePos - iDel + n) % n;
    int   p1   = (p0 - 1 + n) % n;
    return buf[p0] * (1.0f - frac) + buf[p1] * frac;
}

void FxChorus::process(const float* const* inputs,
                       float* const*       outputs,
                       int                 numSamples) {
    const float* in   = inputs[0];
    float*       outL = outputs[0];
    float*       outR = outputs[1];

    if (!in) {
        std::fill(outL, outL + numSamples, 0.0f);
        std::fill(outR, outR + numSamples, 0.0f);
        return;
    }

    float rate      = 0.1f * std::pow(50.0f, params_[kRate]);    // 0.1..5 Hz
    float depthMs   = params_[kDepth] * 15.0f;                   // 0..15ms
    float baseMs    = 1.0f + params_[kDelay] * 29.0f;            // 1..30ms
    float mix       = params_[kMix];
    float dry       = 1.0f - mix;

    float baseDelay = static_cast<float>(baseMs * 0.001 * sampleRate_);
    float depthSamples = static_cast<float>(depthMs * 0.001 * sampleRate_);
    float dt = rate / static_cast<float>(sampleRate_);
    int   n  = static_cast<int>(buf_.size());

    for (int i = 0; i < numSamples; ++i) {
        buf_[writePos_] = in[i];

        float sumL = 0.0f, sumR = 0.0f;
        for (int v = 0; v < kNumVoices; ++v) {
            float lfo = std::sin(2.0f * std::numbers::pi_v<float> * lfoPhase_[v]);
            float del = std::clamp(baseDelay + lfo * depthSamples, 1.0f, static_cast<float>(n - 1));
            float s   = readInterp(buf_, writePos_, del);

            // L: głosy 0,1,2 równo; R: z odwróconą fazą głosu 1 (efekt stereo)
            sumL += s;
            sumR += (v == 1) ? -s : s;

            lfoPhase_[v] += dt;
            if (lfoPhase_[v] >= 1.0f) lfoPhase_[v] -= 1.0f;
        }
        sumL /= kNumVoices;
        sumR /= kNumVoices;

        if (++writePos_ >= n) writePos_ = 0;

        outL[i] = dry * in[i] + mix * sumL;
        outR[i] = dry * in[i] + mix * sumR;
    }
}

void FxChorus::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
