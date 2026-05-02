#pragma once
#include "engine/Module.h"

namespace synth {

// ── EnvADSR — koperta z krzywymi wykładniczymi ────────────────────────────────
// Identyczna charakterystyka czasowa jak w Nord Modular G2:
//   - Krzywe wykładnicze (nie liniowe) = bardziej muzyczne zachowanie
//   - Atak: 0.5ms..30s, Decay: 0.5ms..30s, Release: 0.5ms..30s
//   - Sustain: 0..1 (poziom, nie czas)
//   - Retrigger: atak startuje od aktualnego poziomu (bez klikania)
//
// Porty wejściowe:  [0] Gate (CV > 0.5 = Note On)
// Porty wyjściowe:  [0] Env CV, [1] Inverted Env CV
class EnvADSR : public Module {
public:
    enum Param {
        kAttack,   // [0..1] → 0.5ms..30s (log)
        kDecay,    // [0..1] → 0.5ms..30s (log)
        kSustain,  // [0..1] → poziom 0..1
        kRelease,  // [0..1] → 0.5ms..30s (log)
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

    // Bezpośrednie wyzwolenie z VoicePool (gdy nie ma kabla Gate)
    void noteOn()  { gateManual_ = true; }
    void noteOff() { gateManual_ = false; }

    std::vector<PortInfo> getPorts() const override;
    std::string           getName()  const override { return "EnvADSR"; }

private:
    // Przelicz czas [0..1] → współczynnik RC per-sample
    float timeToCoeff(float param) const noexcept;

    // Zakresy czasu: 0.5ms..30s
    static constexpr float kMinTimeMs = 0.5f;
    static constexpr float kMaxTimeMs = 30000.0f;

    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    double sampleRate_ = 96000.0;
    Stage  stage_      = Stage::Idle;
    float  value_      = 0.0f;   // aktualny poziom koperty
    bool   prevGate_   = false;  // poprzedni stan gate (detekcja zbocza)
    bool   gateManual_ = false;  // gate z VoicePool (bez kabla)

    float params_[kNumParams] = { 0.1f, 0.3f, 0.7f, 0.4f };
    //                             ~10ms  ~1s   70%   ~300ms
};

} // namespace synth
