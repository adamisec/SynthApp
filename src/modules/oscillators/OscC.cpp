#include "OscC.h"
#include <algorithm>
#include <numbers>
#include <cmath>

namespace synth {

// ── Budowanie tablic wavetable ────────────────────────────────────────────────
//
// Tablice (w kolejności kTable 0..7):
//  0 — Sine
//  1 — Triangle
//  2 — Sawtooth (addytywna synteza, 8 harmonicznych)
//  3 — Square   (addytywna synteza, nieparzyste harmoniczne)
//  4 — Soft saw  (pierwsze 4 harmoniczne)
//  5 — Bright square (4 nieparzyste)
//  6 — "Organ"  (1 + 2 + 3 harmoniczne równe amplitudy)
//  7 — "Brass"  (narastające harmoniczne 1..6)

void OscC::buildTables() {
    const float twoPi = 2.0f * std::numbers::pi_v<float>;

    for (int n = 0; n < kTableSize; ++n) {
        float t = static_cast<float>(n) / kTableSize;

        // 0: Sine
        tables_[0][n] = std::sin(twoPi * t);

        // 1: Triangle
        tables_[1][n] = (t < 0.5f) ? (4.0f * t - 1.0f) : (3.0f - 4.0f * t);

        // 2: Sawtooth (addytywna, 8 harm.) — -6dB/oct amplitudy
        {
            float s = 0.0f;
            for (int h = 1; h <= 8; ++h)
                s += std::sin(twoPi * h * t) / h;
            tables_[2][n] = s * (2.0f / std::numbers::pi_v<float>);
        }

        // 3: Square (nieparzyste harmoniczne, 8 term.)
        {
            float s = 0.0f;
            for (int h = 1; h <= 15; h += 2)
                s += std::sin(twoPi * h * t) / h;
            tables_[3][n] = s * (4.0f / std::numbers::pi_v<float>);
        }

        // 4: Soft saw (pierwsze 4 harmoniczne)
        {
            float s = 0.0f;
            for (int h = 1; h <= 4; ++h)
                s += std::sin(twoPi * h * t) / h;
            tables_[4][n] = s * (2.0f / std::numbers::pi_v<float>);
        }

        // 5: Bright square (4 nieparzyste)
        {
            float s = 0.0f;
            for (int h = 1; h <= 7; h += 2)
                s += std::sin(twoPi * h * t) / h;
            tables_[5][n] = s * (4.0f / std::numbers::pi_v<float>);
        }

        // 6: "Organ" (1 + 2 + 3, równe amplitudy)
        tables_[6][n] = (std::sin(twoPi * 1 * t)
                       + std::sin(twoPi * 2 * t)
                       + std::sin(twoPi * 3 * t)) / 3.0f;

        // 7: "Brass" (narastające amplitudy harmonicznych 1..6)
        {
            float s = 0.0f, sum = 0.0f;
            for (int h = 1; h <= 6; ++h) {
                s   += std::sin(twoPi * h * t) * h;
                sum += h;
            }
            tables_[7][n] = s / sum;
        }
    }

    // Normalizacja każdej tablicy do zakresu ±1
    for (int i = 0; i < kNumTables; ++i) {
        float peak = 0.0f;
        for (float v : tables_[i]) peak = std::max(peak, std::abs(v));
        if (peak > 0.001f)
            for (float& v : tables_[i]) v /= peak;
    }

    tablesBuilt_ = true;
}

float OscC::readTable(int idx, float phase) const noexcept {
    idx = std::clamp(idx, 0, kNumTables - 1);
    float pos  = phase * kTableSize;
    int   i0   = static_cast<int>(pos) & (kTableSize - 1);
    int   i1   = (i0 + 1)              & (kTableSize - 1);
    float frac = pos - static_cast<int>(pos);
    return tables_[idx][i0] + frac * (tables_[idx][i1] - tables_[idx][i0]);
}

void OscC::prepare(double sampleRate, int /*blockSize*/) {
    sampleRate_ = sampleRate;
    if (!tablesBuilt_) buildTables();
}

std::vector<PortInfo> OscC::getPorts() const {
    return {
        { "Pitch CV",  SignalType::CV,    false },  // in 0
        { "Morph CV",  SignalType::CV,    false },  // in 1
        { "Audio Out", SignalType::Audio, true  },  // out 0
    };
}

void OscC::process(const float* const* inputs,
                   float* const*       outputs,
                   int                 numSamples) {
    const float* pitchCV = inputs[0];
    const float* morphCV = inputs[1];
    float*       out     = outputs[0];

    float midiNote = params_[kPitch] * 127.0f + (params_[kFine] - 0.5f) * 2.0f;

    // Tabela bazowa i morph
    float tableF  = params_[kTable] * (kNumTables - 1);
    int   tableA  = static_cast<int>(tableF);
    float morphT  = tableF - tableA;
    int   tableB  = std::min(tableA + 1, kNumTables - 1);

    for (int i = 0; i < numSamples; ++i) {
        float note = midiNote;
        if (pitchCV) note += pitchCV[i] * 12.0f;

        float freq = midiToHz(note);
        freq = std::clamp(freq, 1.0f, static_cast<float>(sampleRate_ * 0.49));

        float morphMod = morphT;
        if (morphCV) morphMod = std::clamp(morphT + morphCV[i] * 0.5f, 0.0f, 1.0f);

        float sA = readTable(tableA, phase_);
        float sB = readTable(tableB, phase_);
        out[i]   = sA + morphMod * (sB - sA);

        phase_ += freq / static_cast<float>(sampleRate_);
        if (phase_ >= 1.0f) phase_ -= 1.0f;
    }
}

void OscC::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
