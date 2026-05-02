#include "OscA.h"
#include <algorithm>
#include <numbers>
#include <cmath>

namespace synth {

// ── Przygotowanie ─────────────────────────────────────────────────────────────

void OscA::prepare(double sampleRate, int /*blockSize*/) {
    sampleRate_ = sampleRate;
}

// ── Porty ─────────────────────────────────────────────────────────────────────

std::vector<PortInfo> OscA::getPorts() const {
    return {
        { "Pitch CV", SignalType::CV,    false },  // wejście 0
        { "PWM CV",   SignalType::CV,    false },  // wejście 1
        { "Audio Out",SignalType::Audio, true  },  // wyjście 0
    };
}

// ── PolyBLEP ──────────────────────────────────────────────────────────────────
// Źródło: Valimaki & Pakarinen (2007)
// t   — faza [0..1]
// dt  — przyrost fazy na próbkę (freq / sampleRate)
//
// Zwraca korektę do dodania/odjęcia przy nieciągłości:
//   +polyblep przy przejściu 0→1 (piła, opadanie)
//   -polyblep przy przejściu 0→-1 (prostokąt, narastanie)

float OscA::polyblep(float t, float dt) const noexcept {
    if (t < dt) {
        // Okolica nieciągłości na początku okresu
        float u = t / dt;
        return u + u - u * u - 1.0f;
    }
    if (t > 1.0f - dt) {
        // Okolica nieciągłości na końcu okresu
        float u = (t - 1.0f) / dt;
        return u * u + u + u + 1.0f;
    }
    return 0.0f;
}

// ── Generowanie próbki ────────────────────────────────────────────────────────

float OscA::generateSample(float t, float dt, int wave, float pwm) const noexcept {
    switch (wave) {
        case 0: // Sine — czyste, brak aliasingu
            return std::sin(t * 2.0f * std::numbers::pi_v<float>);

        case 1: { // Triangle — całkowanie piły, wygładzony
            float tri = (t < 0.5f) ? (4.0f * t - 1.0f) : (3.0f - 4.0f * t);
            return tri;
        }

        case 2: { // Sawtooth — piła narastająca + PolyBLEP
            float saw = 2.0f * t - 1.0f;
            saw -= polyblep(t, dt);
            return saw;
        }

        case 3: { // Square (50%) + PolyBLEP
            float sq = (t < 0.5f) ? 1.0f : -1.0f;
            sq += polyblep(t, dt);
            sq -= polyblep(std::fmod(t + 0.5f, 1.0f), dt);
            return sq;
        }

        case 4: { // Pulse z PWM + PolyBLEP
            float pw = std::clamp(pwm, 0.05f, 0.95f);
            float sq = (t < pw) ? 1.0f : -1.0f;
            sq += polyblep(t, dt);
            sq -= polyblep(std::fmod(t + (1.0f - pw), 1.0f), dt);
            // DC offset compensation
            sq -= (2.0f * pw - 1.0f);
            return sq;
        }

        default:
            return 0.0f;
    }
}

// ── Przetwarzanie ─────────────────────────────────────────────────────────────

void OscA::process(const float* const* inputs,
                   float* const*       outputs,
                   int                 numSamples) {
    const float* pitchCV = inputs[0];   // nullptr = brak kabla
    const float* pwmCV   = inputs[1];

    float* out = outputs[0];

    // Oblicz nutę MIDI z parametru (0..1 → 0..127)
    float midiNote = params_[kPitch] * 127.0f
                   + (params_[kFine] - 0.5f) * 2.0f;  // ±1 półton fine

    // Kwantyzacja kształtu (5 wartości)
    int wave = static_cast<int>(params_[kWaveform] * 4.99f);

    float pwm = params_[kPWM];

    for (int i = 0; i < numSamples; ++i) {
        // Pitch CV [0..1] = nuta MIDI 0..127 (absolute, z KeyNote)
        float noteWithCV = midiNote;
        if (pitchCV) noteWithCV = pitchCV[i] * 127.0f + (params_[kFine] - 0.5f) * 2.0f;

        float freq = midiToHz(noteWithCV);
        freq = std::clamp(freq, 1.0f, static_cast<float>(sampleRate_ * 0.49));

        float dt = freq / static_cast<float>(sampleRate_);

        float pwmMod = pwm;
        if (pwmCV) pwmMod = std::clamp(pwm + pwmCV[i] * 0.5f, 0.05f, 0.95f);

        out[i] = generateSample(phase_, dt, wave, pwmMod);

        phase_ += dt;
        if (phase_ >= 1.0f) phase_ -= 1.0f;
    }
}

// ── Parametry ─────────────────────────────────────────────────────────────────

void OscA::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
