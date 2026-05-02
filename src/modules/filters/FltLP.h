#pragma once
#include "engine/Module.h"
#include <array>
#include <cmath>

namespace synth {

// ── FltLP — Zero-Delay Feedback Ladder Filter ────────────────────────────────
// Implementacja wg Zavalishin "The Art of VA Filter Design" §6.5
//
// Charakterystyka:
//   - 4-biegunowy filtr dolnoprzepustowy (-24 dB/okt)
//   - Nieliniowość tanh w każdej sekcji (ciepłe nasycenie przy rezonansie)
//   - Samooscylacja przy resonance = 1.0
//   - Śledzenie klawiatury (cutoff skalowany przez CV)
//
// Porty wejściowe:  [0] Audio In, [1] Cutoff CV, [2] Resonance CV
// Porty wyjściowe:  [0] Audio Out (LP 4-pole), [1] LP 2-pole (środkowy tap)
class FltLP : public Module {
public:
    enum Param {
        kCutoff,      // [0..1] → 20 Hz..20 kHz (log)
        kResonance,   // [0..1] → 0..4 (przy 1.0 = samooscylacja)
        kKbdTrack,    // [0..1] → śledzenie klawiatury (0 = off, 1 = 100%)
        kInputGain,   // [0..1] → wzmocnienie wejścia (0.5 = 0dB, 1.0 = +6dB)
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
    std::string           getName()  const override { return "FltLP"; }

private:
    // ZDF: jedna sekcja one-pole z tanh
    float processOnePole(float x, float g, float& s) noexcept;

    // Szybki przybliżony tanh (błąd < 0.5% przy |x| < 4)
    static float fastTanh(float x) noexcept;

    // 20 Hz * (20000/20)^param = 20 Hz..20 kHz
    static float cutoffHz(float param, double sr) noexcept {
        return 20.0f * std::pow(1000.0f, param);
    }

    double sampleRate_ = 96000.0;

    // Stany 4 sekcji (ZDF)
    std::array<float, 4> s_ = {};

    float params_[kNumParams] = { 0.7f, 0.2f, 0.0f, 0.5f };
    //                             ~3kHz  low   noKbd   0dB
};

} // namespace synth
