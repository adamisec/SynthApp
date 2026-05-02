#include "KeyNote.h"
#include <algorithm>
#include <cstring>

namespace synth {

std::vector<PortInfo> KeyNote::getPorts() const {
    return {
        { "Pitch CV",  SignalType::CV,   true },  // out 0
        { "Gate",      SignalType::Gate, true },  // out 1
        { "Velocity",  SignalType::CV,   true },  // out 2
        { "Note#",     SignalType::CV,   true },  // out 3
    };
}

void KeyNote::process(const float* const* /*inputs*/,
                      float* const*       outputs,
                      int                 numSamples) {
    float note  = params_[kNote];      // 0..1
    float vel   = params_[kVelocity];
    float gate  = params_[kGate];

    // Wypełnij bufory stałą wartością przez cały blok
    // (zmiany nuty/gate zawsze na granicy bloku)
    for (int i = 0; i < numSamples; ++i) {
        outputs[0][i] = note;   // Pitch CV
        outputs[1][i] = gate;   // Gate
        outputs[2][i] = vel;    // Velocity
        outputs[3][i] = note * 127.0f;  // Note# (0..127)
    }
}

void KeyNote::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
