#pragma once
#include "engine/Module.h"
#include <cmath>

namespace synth {

// ── EnvAD — koperta Attack/Decay (one-shot) ───────────────────────────────────
// Wyzwalana zboczem gate. Po zakończeniu Decay wraca do 0 bez Sustain.
// Tryb Loop: po Decay automatycznie restartuje Attack (LFO-like).
//
// Porty wejściowe:  [0] Gate/Trigger
// Porty wyjściowe:  [0] Env Out, [1] Inverted Env Out
class EnvAD : public Module {
public:
    enum Param {
        kAttack,   // [0..1] → 0.5ms..30s (log)
        kDecay,    // [0..1] → 0.5ms..30s (log)
        kLoop,     // [0..1] → < 0.5 = one-shot, ≥ 0.5 = loop
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

    void noteOn()  { triggered_ = true; }

    std::vector<PortInfo> getPorts() const override;
    std::string           getName()  const override { return "EnvAD"; }

private:
    float timeToCoeff(float param) const noexcept;

    static constexpr float kMinTimeMs = 0.5f;
    static constexpr float kMaxTimeMs = 30000.0f;

    enum class Stage { Idle, Attack, Decay };

    double sampleRate_ = 96000.0;
    Stage  stage_      = Stage::Idle;
    float  value_      = 0.0f;
    bool   prevGate_   = false;
    bool   triggered_  = false;

    float params_[kNumParams] = { 0.05f, 0.4f, 0.0f };
    //                             ~5ms   ~3s   one-shot
};

} // namespace synth
