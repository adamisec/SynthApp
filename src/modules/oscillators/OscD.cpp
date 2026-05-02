#include "OscD.h"
#include <algorithm>
#include <numbers>
#include <cmath>

namespace synth {

void OscD::prepare(double sampleRate, int /*blockSize*/) {
    sampleRate_ = sampleRate;
}

std::vector<PortInfo> OscD::getPorts() const {
    return {
        { "Pitch CV", SignalType::CV,    false },  // in 0
        { "Index CV", SignalType::CV,    false },  // in 1
        { "Audio Out",SignalType::Audio, true  },  // out 0
    };
}

void OscD::process(const float* const* inputs,
                   float* const*       outputs,
                   int                 numSamples) {
    const float* pitchCV = inputs[0];
    const float* indexCV = inputs[1];
    float*       out     = outputs[0];

    const float twoPi = 2.0f * std::numbers::pi_v<float>;

    float midiNote = params_[kPitch] * 127.0f + (params_[kFine] - 0.5f) * 2.0f;
    float ratio    = ratioFromParam(params_[kRatio]);
    float index    = params_[kIndex] * 10.0f;  // 0..10
    float feedback = params_[kFeedback] * std::numbers::pi_v<float>;  // 0..π

    for (int i = 0; i < numSamples; ++i) {
        float note = midiNote;
        if (pitchCV) note += pitchCV[i] * 12.0f;

        float fc = midiToHz(note);
        fc = std::clamp(fc, 1.0f, static_cast<float>(sampleRate_ * 0.49));
        float fm = fc * ratio;

        float dtC = fc / static_cast<float>(sampleRate_);
        float dtM = fm / static_cast<float>(sampleRate_);

        float indexMod = index;
        if (indexCV) indexMod = std::clamp(index + indexCV[i] * 5.0f, 0.0f, 10.0f);

        // Modulator z feedbackiem
        float modSample  = std::sin(twoPi * phaseM_ + feedback * lastMod_);
        lastMod_ = modSample;

        // Carrier zmodulowany przez modulator
        out[i] = std::sin(twoPi * phaseC_ + indexMod * modSample);

        phaseC_ += dtC;
        phaseM_ += dtM;
        if (phaseC_ >= 1.0f) phaseC_ -= 1.0f;
        if (phaseM_ >= 1.0f) phaseM_ -= 1.0f;
    }
}

void OscD::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
