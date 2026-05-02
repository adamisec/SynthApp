#include "FxCompressor.h"
#include <algorithm>
#include <cmath>

namespace synth {

void FxCompressor::prepare(double sampleRate, int /*blockSize*/) {
    sampleRate_ = sampleRate;
    // Przelicz attack/release na współczynniki IIR
    setParam(kAttack,  params_[kAttack]);
    setParam(kRelease, params_[kRelease]);
    reset();
}

std::vector<PortInfo> FxCompressor::getPorts() const {
    return {
        { "Audio In",  SignalType::Audio, false },
        { "Sidechain", SignalType::Audio, false },
        { "Audio Out", SignalType::Audio, true  },
        { "GR CV",     SignalType::CV,    true  },
    };
}

float FxCompressor::computeGain(float levelDb) const noexcept {
    float threshold = -60.0f + params_[kThreshold] * 60.0f; // -60..0 dBFS
    float ratio     = 1.0f + std::pow(99.0f, params_[kRatio]);  // 1..100:1
    float knee      = params_[kKnee] * 12.0f;  // 0..12 dB

    float overshoot = levelDb - threshold;

    if (knee > 0.0f && std::abs(overshoot) < knee * 0.5f) {
        // Soft knee: interpolacja kwadratowa
        float kneeFactor = (overshoot + knee * 0.5f) / knee;
        overshoot = kneeFactor * kneeFactor * knee * 0.5f;
    }

    if (overshoot <= 0.0f) return 1.0f; // poniżej progu

    float gainDb = overshoot * (1.0f / ratio - 1.0f);
    return dbToLin(gainDb);
}

void FxCompressor::process(const float* const* inputs,
                           float* const*       outputs,
                           int                 numSamples) {
    const float* in  = inputs[0];
    const float* sc  = inputs[1]; // sidechain (lub nullptr)
    float*       out = outputs[0];
    float*       grOut = outputs[1];

    if (!in) {
        std::fill(out, out + numSamples, 0.0f);
        if (grOut) std::fill(grOut, grOut + numSamples, 0.0f);
        return;
    }

    bool  useRms   = params_[kDetect] >= 0.5f;
    float makeup   = dbToLin(params_[kMakeup] * 24.0f);

    for (int i = 0; i < numSamples; ++i) {
        float sig = sc ? sc[i] : in[i];

        // Detekcja poziomu
        float level;
        if (useRms) {
            rmsAccum_ += sig * sig;
            level = std::sqrt(rmsAccum_);
            rmsAccum_ *= 0.9999f; // uproszczony RMS (czas integracji ~10ms)
        } else {
            level = std::abs(sig);
        }

        // Envelope follower (peak)
        if (level > envFollower_)
            envFollower_ += attCoeff_ * (level - envFollower_);
        else
            envFollower_ += relCoeff_ * (level - envFollower_);

        // Gain computer
        float desiredGain = computeGain(linToDb(envFollower_));

        // Gain smoother
        if (desiredGain < gainSmooth_)
            gainSmooth_ += attCoeff_ * (desiredGain - gainSmooth_);
        else
            gainSmooth_ += relCoeff_ * (desiredGain - gainSmooth_);

        out[i] = in[i] * gainSmooth_ * makeup;

        if (grOut) grOut[i] = gainSmooth_ - 1.0f; // 0..-1
    }
}

void FxCompressor::setParam(int i, float v) {
    if (i < 0 || i >= kNumParams) return;
    params_[i] = std::clamp(v, 0.0f, 1.0f);

    if (i == kAttack) {
        float ms = 0.1f * std::pow(1000.0f, params_[kAttack]); // 0.1..100ms
        attCoeff_ = 1.0f - std::exp(-1.0f / (ms * 0.001f * static_cast<float>(sampleRate_)));
    }
    if (i == kRelease) {
        float ms = 5.0f * std::pow(400.0f, params_[kRelease]); // 5..2000ms
        relCoeff_ = 1.0f - std::exp(-1.0f / (ms * 0.001f * static_cast<float>(sampleRate_)));
    }
}

} // namespace synth
