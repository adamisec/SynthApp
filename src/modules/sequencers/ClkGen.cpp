#include "ClkGen.h"
#include <algorithm>
#include <cmath>

namespace synth {

// Mapowanie kParam → BPM: 0→40, 0.5→120, 1→240 (log)
static double paramToBPM(float p) {
    return 40.0 * std::pow(240.0 / 40.0, static_cast<double>(p));
}

// Mapowanie kDivision → mnożnik taktów na ćwierćnutę
// 0=1/32, 1/6=1/16, 2/6=1/8, 3/6=1/4, 4/6=1/2, 5/6=1/1
static double divisionMult(float p) {
    int idx = static_cast<int>(p * 5.99f);
    const double table[] = { 8.0, 4.0, 2.0, 1.0, 0.5, 0.25 };
    return table[std::clamp(idx, 0, 5)];
}

void ClkGen::prepare(double sampleRate, int /*blockSize*/) {
    sampleRate_ = sampleRate;
    reset();
}

std::vector<PortInfo> ClkGen::getPorts() const {
    return {
        { "Clock Out",   SignalType::Gate, true },  // wyjście 0
        { "Quarter Out", SignalType::Gate, true },  // wyjście 1
    };
}

void ClkGen::process(const float* const* /*inputs*/,
                     float* const*       outputs,
                     int                 numSamples) {
    float* clkOut  = outputs[0];
    float* beatOut = outputs[1];

    double bpm  = (hostBPM_ > 0.0) ? hostBPM_ : paramToBPM(params_[kBPM]);
    double mult = divisionMult(params_[kDivision]);

    // Przyrost fazy na próbkę dla wybranego podziału
    double clkDt  = bpm * mult / (60.0 * sampleRate_);
    // Przyrost fazy na próbkę dla ćwierćnuty
    double beatDt = bpm / (60.0 * sampleRate_);

    for (int i = 0; i < numSamples; ++i) {
        // Clock output
        if (phase_ >= 1.0) {
            phase_ -= 1.0;
            pulseCounter_ = kPulseLength;
        }
        clkOut[i] = (pulseCounter_ > 0) ? 1.0f : 0.0f;
        if (pulseCounter_ > 0) --pulseCounter_;
        phase_ += clkDt;

        // Quarter-note output
        if (beatPhase_ >= 1.0) {
            beatPhase_ -= 1.0;
            beatPulseCounter_ = kPulseLength;
        }
        beatOut[i] = (beatPulseCounter_ > 0) ? 1.0f : 0.0f;
        if (beatPulseCounter_ > 0) --beatPulseCounter_;
        beatPhase_ += beatDt;
    }
}

void ClkGen::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
