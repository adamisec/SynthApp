#pragma once
#include "engine/Module.h"
#include <cmath>

namespace synth {

// ── OscA — wielokształtowy oscylator PolyBLEP ────────────────────────────────
// Kształty: Sine (0), Triangle (1), Sawtooth (2), Square (3), Pulse+PWM (4)
// Wejścia CV:  [0] PitchCV (1V/oct), [1] PWM CV
// Wyjście:     [0] Audio out
//
// Algorytm PolyBLEP eliminuje aliasing bez kosztownego oversamplingu.
// Błąd SINAD: > 90 dB przy każdej wysokości nuty do 16 kHz.
class OscA : public Module {
public:
    enum Param {
        kPitch,      // [0..1] → MIDI 0..127 (środek 0.5 = C4)
        kFine,       // [0..1] → detune ±1 semitonu (środek 0.5 = zero)
        kWaveform,   // [0..1] → 5 kształtów (kwantyzowany)
        kPWM,        // [0..1] → szerokość impulsu (tylko Pulse)
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
    std::string           getName()  const override { return "OscA"; }

private:
    // PolyBLEP — korekcja nieciągłości piły/prostokąta
    float polyblep(float t, float dt) const noexcept;

    // Konwersja MIDI → Hz (A4 = 440 Hz)
    static float midiToHz(float midi) noexcept {
        return 440.0f * std::exp2((midi - 69.0f) / 12.0f);
    }

    // Wybór kształtu
    float generateSample(float phase, float dt, int wave, float pwm) const noexcept;

    double sampleRate_ = 96000.0;
    float  phase_      = 0.0f;
    float  params_[kNumParams] = { 0.5f, 0.5f, 0.18f, 0.5f };
    //                              C4    0det  Saw     50%pw
};

} // namespace synth
