#pragma once
#include "engine/Module.h"
#include <cmath>
#include <array>

namespace synth {

// ── SeqNote — 16-krokowy sekwencer melodyczny ────────────────────────────────
// Wejścia:
//   [0] Clock In  — impuls gate uruchamia następny krok
//   [1] Reset In  — impuls gate resetuje do kroku 1
// Wyjścia:
//   [0] Note CV   — nuta w V/oct (0=C4, +1=C5, -1=C4)
//   [1] Gate Out  — gate 1/0 (aktywny gdy krok nie jest "rest")
//
// Parametry:
//   kStep0..kStep15  — wysokość nuty [0..1] → MIDI 0..127
//   kGate0..kGate15  — 0.0=rest, 0.5=gate, 1.0=tie (legato)
//   kLength          — długość sekwencji [0..1] → 1..16 kroków
//   kGlide           — czas glide [0..1] → 0..500ms
class SeqNote : public Module {
public:
    static constexpr int kSteps = 16;

    enum Param {
        // Step notes: 16 × [0..1] → MIDI 0..127
        kStep0, kStep1, kStep2,  kStep3,
        kStep4, kStep5, kStep6,  kStep7,
        kStep8, kStep9, kStep10, kStep11,
        kStep12, kStep13, kStep14, kStep15,
        // Step gates: 16 × 0.0=rest, 0.33=short, 0.66=normal, 1.0=tie
        kGate0, kGate1, kGate2,  kGate3,
        kGate4, kGate5, kGate6,  kGate7,
        kGate8, kGate9, kGate10, kGate11,
        kGate12, kGate13, kGate14, kGate15,
        kLength,   // [0..1] → 1..16 kroków
        kGlide,    // [0..1] → 0..500ms glide time
        kNumParams
    };

    SeqNote();

    void prepare(double sampleRate, int blockSize) override;
    void reset()   override;

    void  process(const float* const* inputs,
                  float* const*       outputs,
                  int                 numSamples) override;

    void  setParam(int i, float v) override;
    float getParam(int i) const    override { return params_[i]; }
    int   getNumParams()  const    override { return kNumParams; }

    std::vector<PortInfo> getPorts() const override;
    std::string           getName()  const override { return "SeqNote"; }

    // Bezpośredni dostęp do aktualnego kroku (dla UI)
    int currentStep() const { return step_; }

private:
    double sampleRate_    = 96000.0;
    int    step_          = 0;
    bool   prevClock_     = false;
    bool   prevReset_     = false;
    float  currentNote_   = 0.5f;   // znormalizowana nuta przed konwersją
    float  targetNote_    = 0.5f;
    float  glideEnv_      = 0.0f;   // aktualny pitch po glide [0..1]
    int    gateCounter_   = 0;
    static constexpr int kGateLength = 960;  // 10ms @ 96kHz

    float params_[kNumParams];

    float noteToCV(float normNote) const;
};

} // namespace synth
