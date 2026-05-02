#pragma once
#include "engine/Module.h"
#include <array>
#include <cmath>

namespace synth {

// ── FltVoice — filtr formantowy (synteza samogłosek) ─────────────────────────
// 3 równoległe filtry pasmowoprzepustowe (F1, F2, F3).
// 5 samogłosek: A, E, I, O, U — morph przez parametr Vowel.
// Formant frequencies wg Petersona & Barney (1952).
//
// Wejścia: [0] Audio In, [1] Vowel CV
// Wyjścia: [0] Audio Out
class FltVoice : public Module {
public:
    enum Param {
        kVowel,   // [0..1] → ciągły morph przez A→E→I→O→U
        kQ,       // [0..1] → szerokość pasm formantów (0.3..3.0)
        kMix,     // [0..1] → drywet filtra formantowego
        kNumParams
    };

    void prepare(double sampleRate, int blockSize) override;
    void reset()   override;

    void  process(const float* const* inputs,
                  float* const*       outputs,
                  int                 numSamples) override;

    void  setParam(int i, float v) override;
    float getParam(int i) const    override { return params_[i]; }
    int   getNumParams()  const    override { return kNumParams; }

    std::vector<PortInfo> getPorts() const override;
    std::string           getName()  const override { return "FltVoice"; }

private:
    // Stan jednego filtra BP drugiego rzędu (biquad)
    struct Biquad {
        float b0 = 0, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        float z1 = 0, z2 = 0;

        // Projekt: filtr pasmowo-przepustowy wg Audio EQ Cookbook
        void design(float freq, float Q, double sr);
        float tick(float x) noexcept;
    };

    // 3 formanty × (interpolacja między dwoma samogłoskami)
    std::array<Biquad, 3> biquads_;

    double sampleRate_ = 96000.0;

    // Tablica częstotliwości formantów [samogłoska][formant] w Hz
    // Kolejność: A, E, I, O, U
    static constexpr float kFormants[5][3] = {
        { 800,  1200, 2800 },  // A
        { 400,  2200, 2600 },  // E
        { 300,  2600, 3000 },  // I
        { 500,   700, 2800 },  // O
        { 300,   800, 2200 },  // U
    };

    float params_[kNumParams] = { 0.0f, 0.5f, 0.8f };
};

} // namespace synth
