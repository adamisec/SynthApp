#include "FxDelay.h"
#include <algorithm>
#include <cmath>

namespace synth {

void FxDelay::prepare(double sampleRate, int /*blockSize*/) {
    sampleRate_ = sampleRate;
    int maxSamples = static_cast<int>(kMaxDelayMs * 0.001 * sampleRate) + 2;
    bufL_.assign(maxSamples, 0.0f);
    bufR_.assign(maxSamples, 0.0f);
    writePosL_ = writePosR_ = 0;
}

void FxDelay::reset() {
    std::fill(bufL_.begin(), bufL_.end(), 0.0f);
    std::fill(bufR_.begin(), bufR_.end(), 0.0f);
    writePosL_ = writePosR_ = 0;
}

std::vector<PortInfo> FxDelay::getPorts() const {
    return {
        { "In L",    SignalType::Audio, false },
        { "In R",    SignalType::Audio, false },
        { "Time CV", SignalType::CV,    false },
        { "Out L",   SignalType::Audio, true  },
        { "Out R",   SignalType::Audio, true  },
    };
}

float FxDelay::readInterp(const std::vector<float>& buf, int writePos, float delayInSamples) const noexcept {
    int   n     = static_cast<int>(buf.size());
    int   iDelay = static_cast<int>(delayInSamples);
    float frac  = delayInSamples - static_cast<float>(iDelay);

    int p0 = (writePos - iDelay + n) % n;
    int p1 = (p0 - 1 + n) % n;
    return buf[p0] * (1.0f - frac) + buf[p1] * frac;
}

void FxDelay::process(const float* const* inputs,
                      float* const*       outputs,
                      int                 numSamples) {
    const float* inL    = inputs[0];
    const float* inR    = inputs[1];
    const float* timeCV = inputs[2];
    float*       outL   = outputs[0];
    float*       outR   = outputs[1];

    bool pingPong = params_[kPingPong] >= 0.5f;
    float feedback = params_[kFeedback] * 0.95f;
    float mix      = params_[kMix];
    float dry      = 1.0f - mix;

    // Czas opóźnienia w próbkach (log 1ms..2000ms)
    float timeMs = 1.0f * std::pow(2000.0f, params_[kTime]);
    float baseDelay = static_cast<float>(timeMs * 0.001 * sampleRate_);
    baseDelay = std::clamp(baseDelay, 1.0f, static_cast<float>(bufL_.size() - 1));

    int n = static_cast<int>(bufL_.size());

    for (int i = 0; i < numSamples; ++i) {
        float xl = inL ? inL[i] : 0.0f;
        float xr = (inR && !pingPong) ? inR[i] : xl;

        float delaySamples = baseDelay;
        if (timeCV) delaySamples *= std::exp2(timeCV[i]);
        delaySamples = std::clamp(delaySamples, 1.0f, static_cast<float>(n - 1));

        float dL = readInterp(bufL_, writePosL_, delaySamples);
        float dR = readInterp(bufR_, writePosR_, delaySamples);

        float writeL, writeR;
        if (pingPong) {
            // L odbiera z R, R odbiera z L (ping-pong)
            writeL = xl + dR * feedback;
            writeR = dL * feedback;
        } else {
            writeL = xl + dL * feedback;
            writeR = xr + dR * feedback;
        }

        bufL_[writePosL_] = writeL;
        bufR_[writePosR_] = writeR;

        if (++writePosL_ >= n) writePosL_ = 0;
        if (++writePosR_ >= n) writePosR_ = 0;

        outL[i] = dry * xl + mix * dL;
        outR[i] = dry * (inR ? xr : xl) + mix * dR;
    }
}

void FxDelay::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
