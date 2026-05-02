#include "SeqNote.h"
#include <algorithm>
#include <cmath>

namespace synth {

// MIDI nuta 60 = C4 = 0V w V/oct (środek zakresu)
// params_[kStepN] ∈ [0..1] → MIDI 0..127 → offset od C4 w V/oct
float SeqNote::noteToCV(float normNote) const {
    float midi = normNote * 127.0f;
    return (midi - 60.0f) / 12.0f;  // V/oct
}

void SeqNote::prepare(double sampleRate, int /*blockSize*/) {
    sampleRate_ = sampleRate;
}

void SeqNote::reset() {
    step_        = 0;
    prevClock_   = false;
    prevReset_   = false;
    gateCounter_ = 0;
}

std::vector<PortInfo> SeqNote::getPorts() const {
    return {
        { "Clock In",  SignalType::Gate,  false },  // wejście 0
        { "Reset In",  SignalType::Gate,  false },  // wejście 1
        { "Note CV",   SignalType::CV,    true  },  // wyjście 0
        { "Gate Out",  SignalType::Gate,  true  },  // wyjście 1
    };
}

void SeqNote::process(const float* const* inputs,
                      float* const*       outputs,
                      int                 numSamples) {
    const float* clkIn   = inputs  ? inputs[0]  : nullptr;
    const float* rstIn   = inputs  ? inputs[1]  : nullptr;
    float*       noteOut = outputs[0];
    float*       gateOut = outputs[1];

    int length = std::max(1, static_cast<int>(params_[kLength] * 15.99f) + 1);

    // Glide: czas [0..1] → 0..500ms → współczynnik wygładzania
    float glideMs   = params_[kGlide] * 500.0f;
    float glideCoef = (glideMs > 0.0f)
                      ? std::exp(-1.0f / (static_cast<float>(sampleRate_) * glideMs * 0.001f))
                      : 0.0f;

    for (int i = 0; i < numSamples; ++i) {
        // Reset (rising edge)
        bool rst = rstIn && rstIn[i] > 0.5f;
        if (rst && !prevReset_) {
            step_        = 0;
            gateCounter_ = 0;
        }
        prevReset_ = rst;

        // Clock (rising edge)
        bool clk = clkIn && clkIn[i] > 0.5f;
        if (clk && !prevClock_) {
            step_ = (step_ + 1) % length;

            // Sprawdź czy krok nie jest "rest" (gate param < 0.2)
            float gateParam = params_[kGate0 + step_];
            bool isRest = gateParam < 0.15f;
            if (!isRest) {
                targetNote_  = params_[kStep0 + step_];
                gateCounter_ = kGateLength;
            }
        }
        prevClock_ = clk;

        // Glide: interpolacja do targetNote_
        if (glideCoef > 0.0f)
            glideEnv_ = glideCoef * glideEnv_ + (1.0f - glideCoef) * targetNote_;
        else
            glideEnv_ = targetNote_;

        noteOut[i] = noteToCV(glideEnv_);
        gateOut[i] = (gateCounter_ > 0) ? 1.0f : 0.0f;
        if (gateCounter_ > 0) --gateCounter_;
    }
}

void SeqNote::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

// Domyślne wartości: C major scale 2 oktawy
SeqNote::SeqNote() {
    // Defaults: C D E F G A B C (8 kroków) + C D E F G A B C
    const float scale[] = {
        60, 62, 64, 65, 67, 69, 71, 72,
        74, 76, 77, 79, 81, 83, 84, 86
    };
    for (int s = 0; s < kSteps; ++s)
        params_[kStep0 + s] = scale[s] / 127.0f;
    for (int s = 0; s < kSteps; ++s)
        params_[kGate0 + s] = 0.66f;  // normal gate
    params_[kLength] = (8.0f - 1.0f) / 15.0f;  // 8 kroków
    params_[kGlide]  = 0.0f;
    glideEnv_        = params_[kStep0];
    targetNote_      = params_[kStep0];
}

} // namespace synth
