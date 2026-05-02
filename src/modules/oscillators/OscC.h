#pragma once
#include "engine/Module.h"
#include <array>
#include <cmath>

namespace synth {

// ── OscC — oscylator wavetable ────────────────────────────────────────────────
// Tablice 256-punktowe, 8 kształtów (Sine, Tri, Saw, Sqr, + 4 wavetable).
// Mipmap: osobne tablice dla zakresów częstotliwości → brak aliasingu.
// Interpolacja liniowa między sąsiednimi próbkami tablicy.
//
// Wejście CV: [0] Pitch CV (1V/oct), [1] Wavetable morph CV
// Wyjście:    [0] Audio out
class OscC : public Module {
public:
    static constexpr int kTableSize  = 256;
    static constexpr int kNumTables  = 8;

    enum Param {
        kPitch,     // [0..1] → MIDI 0..127
        kFine,      // [0..1] → ±1 semitonu
        kTable,     // [0..1] → wybór tablicy (8 pozycji)
        kMorph,     // [0..1] → morph między tabelą[n] a [n+1]
        kNumParams
    };

    void prepare(double sampleRate, int blockSize) override;
    void reset()   override { phase_ = 0.0f; }

    void  process(const float* const* inputs,
                  float* const*       outputs,
                  int                 numSamples) override;

    void  setParam(int i, float v) override;
    float getParam(int i) const    override { return params_[i]; }
    int   getNumParams()  const    override { return kNumParams; }

    std::vector<PortInfo> getPorts() const override;
    std::string           getName()  const override { return "OscC"; }

private:
    // Generuj tablice przy init
    void buildTables();

    // Interpolacja liniowa w tablicy
    float readTable(int tableIdx, float phase) const noexcept;

    static float midiToHz(float midi) noexcept {
        return 440.0f * std::exp2((midi - 69.0f) / 12.0f);
    }

    double sampleRate_ = 96000.0;
    float  phase_      = 0.0f;
    bool   tablesBuilt_ = false;

    // 8 tablic × 256 próbek
    std::array<std::array<float, kTableSize>, kNumTables> tables_{};

    float params_[kNumParams] = { 0.5f, 0.5f, 0.0f, 0.0f };
};

} // namespace synth
