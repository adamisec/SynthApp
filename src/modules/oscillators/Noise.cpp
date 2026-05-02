#include "Noise.h"
#include <algorithm>

namespace synth {

std::vector<PortInfo> Noise::getPorts() const {
    return {
        { "Audio Out", SignalType::Audio, true },
    };
}

// Xorshift32 PRNG — deterministyczny, bardzo szybki
float Noise::nextWhite() {
    seed_ ^= seed_ << 13;
    seed_ ^= seed_ >> 17;
    seed_ ^= seed_ << 5;
    // Normalizacja do [-1, 1]
    return static_cast<float>(static_cast<int32_t>(seed_)) / 2147483648.0f;
}

void Noise::process(const float* const* /*inputs*/,
                    float* const*       outputs,
                    int                 numSamples) {
    float* out   = outputs[0];
    float  level = params_[kLevel];
    bool   pink  = params_[kColor] > 0.5f;

    for (int i = 0; i < numSamples; ++i) {
        float w = nextWhite();

        if (pink) {
            // Paul Kellet's pink noise filter — 7-stage IIR
            b0_ = 0.99886f * b0_ + w * 0.0555179f;
            b1_ = 0.99332f * b1_ + w * 0.0750759f;
            b2_ = 0.96900f * b2_ + w * 0.1538520f;
            b3_ = 0.86650f * b3_ + w * 0.3104856f;
            b4_ = 0.55000f * b4_ + w * 0.5329522f;
            b5_ = -0.7616f * b5_ - w * 0.0168980f;
            float pink = (b0_+b1_+b2_+b3_+b4_+b5_+b6_ + w*0.5362f) * 0.11f;
            b6_ = w * 0.115926f;
            out[i] = pink * level;
        } else {
            out[i] = w * level;
        }
    }
}

void Noise::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams)
        params_[i] = std::clamp(v, 0.0f, 1.0f);
}

} // namespace synth
