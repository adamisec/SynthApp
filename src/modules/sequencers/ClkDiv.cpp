#include "ClkDiv.h"
#include <algorithm>

namespace synth {

void ClkDiv::prepare(double sampleRate, int /*blockSize*/) {
    sampleRate_ = sampleRate;
    reset();
}

void ClkDiv::reset() {
    prevClock_ = false;
    counter_   = 0;
    for (int k = 0; k < 5; ++k) {
        counts_[k] = 0;
        pulses_[k] = 0;
    }
}

std::vector<PortInfo> ClkDiv::getPorts() const {
    return {
        { "Clock In",  SignalType::Gate, false },
        { "/2 Out",    SignalType::Gate, true  },
        { "/4 Out",    SignalType::Gate, true  },
        { "/8 Out",    SignalType::Gate, true  },
        { "/16 Out",   SignalType::Gate, true  },
        { "/32 Out",   SignalType::Gate, true  },
    };
}

void ClkDiv::process(const float* const* inputs,
                     float* const*       outputs,
                     int                 numSamples) {
    const float* clkIn = inputs ? inputs[0] : nullptr;

    for (int i = 0; i < numSamples; ++i) {
        bool clk = clkIn && clkIn[i] > 0.5f;
        bool rising = clk && !prevClock_;
        prevClock_ = clk;

        if (rising) {
            ++counter_;
            for (int k = 0; k < 5; ++k) {
                ++counts_[k];
                if (counts_[k] >= kDivs[k]) {
                    counts_[k]  = 0;
                    pulses_[k]  = kPulseLength;
                }
            }
        }

        for (int k = 0; k < 5; ++k) {
            outputs[k][i] = (pulses_[k] > 0) ? 1.0f : 0.0f;
            if (pulses_[k] > 0) --pulses_[k];
        }
    }
}

} // namespace synth
