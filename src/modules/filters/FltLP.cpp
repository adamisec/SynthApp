#include "FltLP.h"
#include <algorithm>
#include <numbers>

namespace synth {

// ── Przybliżony tanh — Padé [3/3] ────────────────────────────────────────────
// Błąd < 0.5% dla |x| < 4.0, niezbędny dla ciągłości DSP w RT
float FltLP::fastTanh(float x) noexcept {
    float x2 = x * x;
    float num = x * (27.0f + x2);
    float den = 27.0f + 9.0f * x2;
    return num / den;
}

// ── Jedna sekcja ZDF one-pole ─────────────────────────────────────────────────
// g  — współczynnik g = tan(π * fc / fs)
// s  — stan integratora
// Zwraca wyjście LP sekcji
inline float FltLP::processOnePole(float x, float g, float& s) noexcept {
    // ZDF: v = (x - s) * g / (1 + g)
    float v = (x - s) * g / (1.0f + g);
    float y = v + s;
    s = y + v;  // aktualizacja stanu
    return y;
}

// ── prepare ───────────────────────────────────────────────────────────────────

void FltLP::prepare(double sampleRate, int /*blockSize*/) {
    sampleRate_ = sampleRate;
    reset();
}

void FltLP::reset() {
    s_.fill(0.0f);
}

// ── Porty ─────────────────────────────────────────────────────────────────────

std::vector<PortInfo> FltLP::getPorts() const {
    return {
        { "Audio In",    SignalType::Audio, false },  // in 0
        { "Cutoff CV",   SignalType::CV,    false },  // in 1
        { "Res CV",      SignalType::CV,    false },  // in 2
        { "Audio Out",   SignalType::Audio, true  },  // out 0
        { "LP 2-pole",   SignalType::Audio, true  },  // out 1 (środkowy tap)
    };
}

// ── Przetwarzanie ─────────────────────────────────────────────────────────────
//
// Implementacja ZDF Ladder wg Zavalishin §6.5:
//
//   Główna pętla globalna (feedback):
//     u   = tanh(gain * x - k * y4)      nieliniowe wejście z feedbackiem
//     y1  = LP1(u,  g, s1)
//     y2  = LP1(y1, g, s2)
//     y3  = LP1(y2, g, s3)
//     y4  = LP1(y3, g, s4)
//
//   Problem: y4 zależy od siebie przez k*y4 → implicit equation.
//   Rozwiązanie analityczne (bez iteracji):
//     Linearyzacja tanh = 1 (małe sygnały) lub LUT przybliżenie.

void FltLP::process(const float* const* inputs,
                    float* const*       outputs,
                    int                 numSamples) {
    const float* audioIn  = inputs[0];
    const float* cutoffCV = inputs[1];
    const float* resCV    = inputs[2];

    float* outLP4 = outputs[0];
    float* outLP2 = outputs[1];

    float baseCutoff = params_[kCutoff];
    float baseRes    = params_[kResonance];
    float inputGain  = 0.5f + params_[kInputGain];  // 0.5..1.5

    for (int i = 0; i < numSamples; ++i) {
        // Modulacja cutoffa przez CV (1V/oct)
        float fc = baseCutoff;
        if (cutoffCV) fc = std::clamp(fc + cutoffCV[i] * 0.2f, 0.0f, 1.0f);

        float res = baseRes;
        if (resCV) res = std::clamp(res + resCV[i] * 0.5f, 0.0f, 1.0f);

        float cutHz = cutoffHz(fc, sampleRate_);
        cutHz = std::clamp(cutHz, 20.0f, static_cast<float>(sampleRate_ * 0.49));

        // g = tan(π * fc / fs)
        float g = std::tan(std::numbers::pi_v<float> * cutHz / static_cast<float>(sampleRate_));
        float k = 4.0f * res;  // 0..4; przy k=4 = samooscylacja

        // Wejście z saturacją + globalny feedback (ZDF exact solution)
        float x = audioIn ? audioIn[i] * inputGain : 0.0f;

        // Poprawna formuła ZDF (Zavalishin §6.5):
        // A = g/(1+g), B = 1/(1+g)
        // y4 = A^4*u + B*(A^3*s0 + A^2*s1 + A*s2 + s3)
        // Rozwiązanie dla u z linearyzowanego feedback: u = (tanh(x) - k*S) / (1 + k*A^4)
        float A  = g / (1.0f + g);
        float B  = 1.0f / (1.0f + g);
        float A2 = A * A;
        float A4 = A2 * A2;
        float S  = B * (A2 * A * s_[0] + A2 * s_[1] + A * s_[2] + s_[3]);
        float den = 1.0f + k * A4;

        float u   = (fastTanh(x) - k * S) / den;

        // Sekcje one-pole
        float y1 = processOnePole(u,  g, s_[0]);
        float y2 = processOnePole(y1, g, s_[1]);
        float y3 = processOnePole(y2, g, s_[2]);
        float y4 = processOnePole(y3, g, s_[3]);

        outLP4[i] = y4;
        outLP2[i] = y2;
    }
}

// ── Parametry ─────────────────────────────────────────────────────────────────

void FltLP::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
