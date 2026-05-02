#include "EnvADSR.h"
#include <algorithm>
#include <cmath>

namespace synth {

// ── Przelicznik czasu → współczynnik RC ──────────────────────────────────────
// param [0..1] → czas w ms (log: 0.5ms..30000ms)
// Współczynnik RC: coeff = exp(-1 / (timeInSamples))
// Krzywa wykładnicza: value += coeff * (target - value)

float EnvADSR::timeToCoeff(float param) const noexcept {
    float ms      = kMinTimeMs * std::pow(kMaxTimeMs / kMinTimeMs, param);
    float samples = ms * static_cast<float>(sampleRate_) / 1000.0f;
    // Prędkość zbieżności RC: mała dla długich czasów, duża dla krótkich
    // value += coeff * (target - value) → 63% w czasie "samples"
    return 1.0f - std::exp(-1.0f / std::max(samples, 1.0f));
}

// ── prepare / reset ───────────────────────────────────────────────────────────

void EnvADSR::prepare(double sampleRate, int /*blockSize*/) {
    sampleRate_ = sampleRate;
    reset();
}

void EnvADSR::reset() {
    stage_    = Stage::Idle;
    value_    = 0.0f;
    prevGate_ = false;
}

// ── Porty ─────────────────────────────────────────────────────────────────────

std::vector<PortInfo> EnvADSR::getPorts() const {
    return {
        { "Gate",         SignalType::Gate, false },  // in 0
        { "Env Out",      SignalType::CV,   true  },  // out 0
        { "Inv Env Out",  SignalType::CV,   true  },  // out 1
    };
}

// ── Przetwarzanie ─────────────────────────────────────────────────────────────
//
// Logika:
//   1. Detekcja zbocza Gate (0→1: NoteOn, 1→0: NoteOff)
//   2. Attack: value → 1.0 (RC z celem 1.0 + epsilon, by osiągnąć szczyt)
//   3. Decay:  value → Sustain
//   4. Sustain: trzymaj poziom
//   5. Release: value → 0.0

void EnvADSR::process(const float* const* inputs,
                      float* const*       outputs,
                      int                 numSamples) {
    const float* gateIn = inputs[0];
    float* envOut    = outputs[0];
    float* invOut    = outputs[1];

    float attackCoeff  = timeToCoeff(params_[kAttack]);
    float decayCoeff   = timeToCoeff(params_[kDecay]);
    float sustainLevel = params_[kSustain];
    float releaseCoeff = timeToCoeff(params_[kRelease]);

    for (int i = 0; i < numSamples; ++i) {
        // Odczyt gate: z kabla lub z ręcznego stanu (z VoicePool)
        bool gate = gateManual_;
        if (gateIn) gate = gateIn[i] > 0.5f;

        // Detekcja zbocza narastającego (Note On)
        if (gate && !prevGate_) {
            stage_ = Stage::Attack;
        }
        // Detekcja zbocza opadającego (Note Off)
        if (!gate && prevGate_) {
            stage_ = Stage::Release;
        }
        prevGate_ = gate;

        // Maszyna stanów koperty
        switch (stage_) {
            case Stage::Idle:
                value_ = 0.0f;
                break;

            case Stage::Attack:
                // Cel = 1.0 + mały nadmiar, by pewnie osiągnąć szczyt
                value_ += attackCoeff * (1.02f - value_);
                if (value_ >= 1.0f) {
                    value_ = 1.0f;
                    stage_ = Stage::Decay;
                }
                break;

            case Stage::Decay:
                value_ += decayCoeff * (sustainLevel - value_);
                // Przejście do Sustain gdy blisko celu
                if (std::abs(value_ - sustainLevel) < 0.0001f) {
                    value_ = sustainLevel;
                    stage_ = Stage::Sustain;
                }
                break;

            case Stage::Sustain:
                value_ = sustainLevel;
                // Wyjście z Sustain tylko przez NoteOff
                break;

            case Stage::Release:
                value_ += releaseCoeff * (0.0f - value_);
                if (value_ < 0.0001f) {
                    value_ = 0.0f;
                    stage_ = Stage::Idle;
                }
                break;
        }

        envOut[i] = value_;
        invOut[i] = 1.0f - value_;
    }
}

// ── Parametry ─────────────────────────────────────────────────────────────────

void EnvADSR::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
