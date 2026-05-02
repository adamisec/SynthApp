#pragma once
#include "engine/Module.h"

namespace synth {

// ── KeyNote — konwerter MIDI → CV + Gate ─────────────────────────────────────
// Jeden na głos. VoicePool ustawia parametry przez setParam() przed każdym
// blokiem. Moduł "tłumaczy" je na sygnały CV/Gate w buforach wyjściowych.
//
// Wyjścia:
//   [0] Pitch CV  — nuta / 127.0 (do OscA wejście PitchCV)
//   [1] Gate      — 0.0 lub 1.0
//   [2] Velocity  — 0..1 (do VCA lub filtra)
//   [3] Note#     — surowy numer nuty 0..127 (do SeqNote, arpeggio)
class KeyNote : public Module {
public:
    enum Param {
        kNote,      // 0..1 = nuta MIDI 0..127
        kVelocity,  // 0..1
        kGate,      // 0.0 lub 1.0
        kNumParams
    };

    void prepare(double sampleRate, int blockSize) override { (void)sampleRate; (void)blockSize; }
    void reset()   override {}

    void  process(const float* const* inputs,
                  float* const*       outputs,
                  int                 numSamples) override;

    void  setParam(int i, float v) override;
    float getParam(int i) const    override { return params_[i]; }
    int   getNumParams()  const    override { return kNumParams; }

    std::vector<PortInfo> getPorts() const override;
    std::string           getName()  const override { return "KeyNote"; }

private:
    float params_[kNumParams] = { 0.5f, 0.8f, 0.0f };
};

} // namespace synth
