#include "FltPhase.h"
#include <algorithm>
#include <numbers>
#include <cmath>

namespace synth {

void FltPhase::prepare(double sampleRate, int /*blockSize*/) {
    sampleRate_ = sampleRate;
    reset();
}

void FltPhase::reset() {
    lfoPhase_   = 0.0f;
    feedback_z_ = 0.0f;
    for (auto& s : stages_) s.z1 = 0.0f;
}

std::vector<PortInfo> FltPhase::getPorts() const {
    return {
        { "Audio In",  SignalType::Audio, false },  // in 0
        { "Rate CV",   SignalType::CV,    false },  // in 1
        { "Audio Out", SignalType::Audio, true  },  // out 0
    };
}

void FltPhase::process(const float* const* inputs,
                       float* const*       outputs,
                       int                 numSamples) {
    const float* audioIn = inputs[0];
    const float* rateCV  = inputs[1];
    float*       out     = outputs[0];

    if (!audioIn) { std::fill(out, out + numSamples, 0.0f); return; }

    // Liczba sekcji: [0..1] → 2/4/6/8
    int numStages = 2 * (1 + static_cast<int>(params_[kStages] * 3.99f));
    numStages = std::clamp(numStages, 2, kMaxStages);

    float feedback = (params_[kFeedback] - 0.5f) * 1.9f;  // -0.95..+0.95
    float mix      = params_[kMix];

    // LFO rate (log: 0.01..10 Hz)
    float rate = 0.01f * std::pow(1000.0f, params_[kRate]);

    // Centrum phasera: 20..8000 Hz (log)
    float freqCenter = 20.0f * std::pow(400.0f, params_[kFreq]);

    // Głębokość modulacji w oktawach
    float depth = params_[kDepth] * 4.0f;

    float dtLFO = rate / static_cast<float>(sampleRate_);

    for (int i = 0; i < numSamples; ++i) {
        float rateMod = rate;
        if (rateCV) rateMod *= std::exp2(rateCV[i]);

        // LFO sinus → modulacja centrum w oktawach
        float lfo = std::sin(2.0f * std::numbers::pi_v<float> * lfoPhase_);
        float fc  = freqCenter * std::exp2(lfo * depth * 0.5f);
        fc = std::clamp(fc, 20.0f, static_cast<float>(sampleRate_ * 0.49));

        // Współczynnik allpass: a = (g - 1) / (g + 1) gdzie g = tan(π*fc/fs)
        float g = std::tan(std::numbers::pi_v<float> * fc / static_cast<float>(sampleRate_));
        float a = (g - 1.0f) / (g + 1.0f);

        // Wejście z feedbackiem
        float x = audioIn[i] + feedback * feedback_z_;

        // Kaskada sekcji allpass
        float ap = x;
        for (int s = 0; s < numStages; ++s)
            ap = stages_[s].tick(ap, a);

        feedback_z_ = ap;

        // Mix: drywet
        out[i] = audioIn[i] * (1.0f - mix) + (audioIn[i] + ap) * 0.5f * mix;

        lfoPhase_ += dtLFO;
        if (lfoPhase_ >= 1.0f) lfoPhase_ -= 1.0f;
    }
}

void FltPhase::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
