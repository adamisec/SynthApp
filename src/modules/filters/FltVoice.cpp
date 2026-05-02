#include "FltVoice.h"
#include <algorithm>
#include <numbers>
#include <cmath>

namespace synth {

// definicja constexpr tablicy
constexpr float FltVoice::kFormants[5][3];

// ── Biquad BP design (Audio EQ Cookbook — BPF constant 0dB peak) ─────────────
void FltVoice::Biquad::design(float freq, float Q, double sr) {
    float w0  = 2.0f * std::numbers::pi_v<float> * freq / static_cast<float>(sr);
    float sn  = std::sin(w0);
    float cs  = std::cos(w0);
    float alpha = sn / (2.0f * Q);

    float a0 =  1.0f + alpha;
    b0 =  alpha / a0;
    b1 =  0.0f;
    b2 = -alpha / a0;
    a1 = -2.0f * cs  / a0;
    a2 = (1.0f - alpha) / a0;
}

float FltVoice::Biquad::tick(float x) noexcept {
    float y = b0 * x + z1;
    z1 = b1 * x - a1 * y + z2;
    z2 = b2 * x - a2 * y;
    return y;
}

// ── prepare / reset ───────────────────────────────────────────────────────────

void FltVoice::prepare(double sampleRate, int /*blockSize*/) {
    sampleRate_ = sampleRate;
    reset();
}

void FltVoice::reset() {
    for (auto& b : biquads_) { b.z1 = b.z2 = 0.0f; }
}

std::vector<PortInfo> FltVoice::getPorts() const {
    return {
        { "Audio In", SignalType::Audio, false },  // in 0
        { "Vowel CV", SignalType::CV,    false },  // in 1
        { "Audio Out",SignalType::Audio, true  },  // out 0
    };
}

// ── Przetwarzanie ─────────────────────────────────────────────────────────────

void FltVoice::process(const float* const* inputs,
                       float* const*       outputs,
                       int                 numSamples) {
    const float* audioIn = inputs[0];
    const float* vowelCV = inputs[1];
    float*       out     = outputs[0];

    if (!audioIn) { std::fill(out, out + numSamples, 0.0f); return; }

    float Q   = 0.3f + params_[kQ] * 2.7f;  // 0.3..3.0
    float mix = params_[kMix];

    // Projekt filtrów na początku bloku (vowel może być modulowany per-blok)
    float vowel     = params_[kVowel];
    if (vowelCV) vowel = std::clamp(vowel + vowelCV[0] * 0.5f, 0.0f, 1.0f);

    // Interpolacja między sąsiednimi samogłoskami
    float vScaled = vowel * 4.0f;  // 0..4
    int   v0      = static_cast<int>(vScaled);
    float vFrac   = vScaled - v0;
    int   v1      = std::min(v0 + 1, 4);

    for (int f = 0; f < 3; ++f) {
        float freq = kFormants[v0][f] + vFrac * (kFormants[v1][f] - kFormants[v0][f]);
        biquads_[f].design(freq, Q, sampleRate_);
    }

    for (int i = 0; i < numSamples; ++i) {
        float x   = audioIn[i];
        // Suma trzech równoległych formantów (F1 dominuje)
        float sum = biquads_[0].tick(x) * 0.6f
                  + biquads_[1].tick(x) * 0.3f
                  + biquads_[2].tick(x) * 0.1f;
        out[i] = x * (1.0f - mix) + sum * mix;
    }
}

void FltVoice::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
