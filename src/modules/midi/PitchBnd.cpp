#include "PitchBnd.h"
#include <algorithm>
#include <cmath>

namespace synth {

std::vector<PortInfo> PitchBnd::getPorts() const {
    return {
        { "Bend CV", SignalType::CV, true },  // wyjście 0
    };
}

void PitchBnd::process(const float* const* /*inputs*/,
                       float* const*       outputs,
                       int                 numSamples) {
    float* out = outputs[0];

    // kRange [0..1] → ±1..±12 półtonów skalowania CV
    // CV 0.5 = brak bendu; odchylenie od 0.5 skalowane przez range
    float bendCV = params_[kBend];  // 0..1, środek = 0.5

    for (int i = 0; i < numSamples; ++i)
        out[i] = bendCV;
}

void PitchBnd::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
