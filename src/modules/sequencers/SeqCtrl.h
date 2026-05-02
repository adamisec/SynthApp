#pragma once
#include "engine/Module.h"
#include <cmath>
#include <array>

namespace synth {

// ── SeqCtrl — 16-krokowy sekwencer CV ────────────────────────────────────────
// Sekwencer CV bez nut — każdy krok ma wartość bipolarną [-1..1].
// Wejścia:
//   [0] Clock In  — impuls gate uruchamia następny krok
//   [1] Reset In  — impuls gate resetuje do kroku 1
// Wyjścia:
//   [0] CV Out    — wartość aktualnego kroku [-1..1]
//   [1] Gate Out  — impuls przy każdej zmianie kroku
//
// Parametry:
//   kStep0..kStep15  — wartość CV [0..1] → bipolarny [-1..1]
//   kLength          — długość sekwencji [0..1] → 1..16 kroków
//   kSmooth          — wygładzanie między krokami [0..1]
class SeqCtrl : public Module {
public:
    static constexpr int kSteps = 16;

    enum Param {
        kStep0, kStep1, kStep2,  kStep3,
        kStep4, kStep5, kStep6,  kStep7,
        kStep8, kStep9, kStep10, kStep11,
        kStep12, kStep13, kStep14, kStep15,
        kLength,   // [0..1] → 1..16 kroków
        kSmooth,   // [0..1] → czas wygładzania
        kNumParams
    };

    SeqCtrl();

    void prepare(double sampleRate, int blockSize) override;
    void reset()   override;

    void  process(const float* const* inputs,
                  float* const*       outputs,
                  int                 numSamples) override;

    void  setParam(int i, float v) override;
    float getParam(int i) const    override { return params_[i]; }
    int   getNumParams()  const    override { return kNumParams; }

    std::vector<PortInfo> getPorts() const override;
    std::string           getName()  const override { return "SeqCtrl"; }

    int currentStep() const { return step_; }

private:
    double sampleRate_   = 96000.0;
    int    step_         = 0;
    bool   prevClock_    = false;
    bool   prevReset_    = false;
    float  currentCV_    = 0.0f;    // wyjście po wygładzeniu
    int    gateCounter_  = 0;
    static constexpr int kGateLength = 960;

    float params_[kNumParams];
};

} // namespace synth
