#include "OscB.h"
#include <algorithm>
#include <numbers>
#include <cmath>

namespace synth {

void OscB::prepare(double sampleRate, int /*blockSize*/) {
    sampleRate_ = sampleRate;
}

std::vector<PortInfo> OscB::getPorts() const {
    return {
        { "Pitch CV",  SignalType::CV,    false },  // wejście 0
        { "PWM CV",    SignalType::CV,    false },  // wejście 1
        { "Sync In",   SignalType::Audio, false },  // wejście 2 — hard sync
        { "Audio Out", SignalType::Audio, true  },  // wyjście 0
    };
}

float OscB::polyblep(float t, float dt) const noexcept {
    if (t < dt) {
        float u = t / dt;
        return u + u - u * u - 1.0f;
    }
    if (t > 1.0f - dt) {
        float u = (t - 1.0f) / dt;
        return u * u + u + u + 1.0f;
    }
    return 0.0f;
}

float OscB::generateSample(float t, float dt, int wave, float pwm) const noexcept {
    switch (wave) {
        case 0:
            return std::sin(t * 2.0f * std::numbers::pi_v<float>);
        case 1:
            return (t < 0.5f) ? (4.0f * t - 1.0f) : (3.0f - 4.0f * t);
        case 2: {
            float saw = 2.0f * t - 1.0f;
            saw -= polyblep(t, dt);
            return saw;
        }
        case 3: {
            float sq = (t < 0.5f) ? 1.0f : -1.0f;
            sq += polyblep(t, dt);
            sq -= polyblep(std::fmod(t + 0.5f, 1.0f), dt);
            return sq;
        }
        case 4: {
            float pw = std::clamp(pwm, 0.05f, 0.95f);
            float sq = (t < pw) ? 1.0f : -1.0f;
            sq += polyblep(t, dt);
            sq -= polyblep(std::fmod(t + (1.0f - pw), 1.0f), dt);
            sq -= (2.0f * pw - 1.0f);
            return sq;
        }
        default: return 0.0f;
    }
}

void OscB::process(const float* const* inputs,
                   float* const*       outputs,
                   int                 numSamples) {
    const float* pitchCV = inputs[0];
    const float* pwmCV   = inputs[1];
    const float* syncIn  = inputs[2];

    float* out = outputs[0];

    float midiNote = params_[kPitch] * 127.0f + (params_[kFine] - 0.5f) * 2.0f;
    int   wave     = static_cast<int>(params_[kWaveform] * 4.99f);
    float pwm      = params_[kPWM];

    for (int i = 0; i < numSamples; ++i) {
        // Pitch CV [0..1] = nuta MIDI 0..127 (absolute, z KeyNote)
        float noteWithCV = midiNote;
        if (pitchCV) noteWithCV = pitchCV[i] * 127.0f + (params_[kFine] - 0.5f) * 2.0f;

        float freq = midiToHz(noteWithCV);
        freq = std::clamp(freq, 1.0f, static_cast<float>(sampleRate_ * 0.49));
        float dt = freq / static_cast<float>(sampleRate_);

        float pwmMod = pwm;
        if (pwmCV) pwmMod = std::clamp(pwm + pwmCV[i] * 0.5f, 0.05f, 0.95f);

        // Hard sync: narastające zbocze na wejściu sync resetuje fazę slave'a
        if (syncIn) {
            bool syncNow = syncIn[i] > 0.5f;
            if (syncNow && !prevSync_) phase_ = 0.0f;
            prevSync_ = syncNow;
        }

        out[i] = generateSample(phase_, dt, wave, pwmMod);

        phase_ += dt;
        if (phase_ >= 1.0f) phase_ -= 1.0f;
    }
}

void OscB::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
