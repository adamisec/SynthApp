#pragma once
#include "engine/Module.h"
#include <cmath>

namespace synth {

// ── OscB — oscylator z hard sync ─────────────────────────────────────────────
// Identyczny zestaw kształtów jak OscA + wejście Sync (resetuje fazę).
// Hard sync: gdy master (zewnętrzny) ukończy okres, faza slave'a wraca do 0.
//
// Wejścia CV:  [0] Pitch CV (1V/oct), [1] PWM CV, [2] Sync In (gate/audio)
// Wyjście:     [0] Audio out
class OscB : public Module {
public:
    enum Param {
        kPitch,     // [0..1] → MIDI 0..127
        kFine,      // [0..1] → detune ±1 semitonu
        kWaveform,  // [0..1] → 5 kształtów (jak OscA)
        kPWM,       // [0..1] → szerokość impulsu
        kNumParams
    };

    void prepare(double sampleRate, int blockSize) override;
    void reset()   override { phase_ = 0.0f; prevSync_ = false; }

    void  process(const float* const* inputs,
                  float* const*       outputs,
                  int                 numSamples) override;

    void  setParam(int i, float v) override;
    float getParam(int i) const    override { return params_[i]; }
    int   getNumParams()  const    override { return kNumParams; }

    std::vector<PortInfo> getPorts() const override;
    std::string           getName()  const override { return "OscB"; }

private:
    float polyblep(float t, float dt) const noexcept;
    float generateSample(float t, float dt, int wave, float pwm) const noexcept;

    static float midiToHz(float midi) noexcept {
        return 440.0f * std::exp2((midi - 69.0f) / 12.0f);
    }

    double sampleRate_ = 96000.0;
    float  phase_      = 0.0f;
    bool   prevSync_   = false;
    float  params_[kNumParams] = { 0.5f, 0.5f, 0.18f, 0.5f };
};

} // namespace synth
