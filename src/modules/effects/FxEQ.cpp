#include "FxEQ.h"
#include <algorithm>
#include <numbers>

namespace synth {

static constexpr float kPi = std::numbers::pi_v<float>;

// ── Low Shelf (Zölzer, DAFX) ─────────────────────────────────────────────────
void FxEQ::calcLowShelf(Biquad& bq, float freqHz, float gainDB, double sr) {
    float A  = std::pow(10.0f, gainDB / 40.0f);  // sqrt(linear gain)
    float w0 = 2.0f * kPi * freqHz / static_cast<float>(sr);
    float cosW = std::cos(w0);
    float sinW = std::sin(w0);
    float S  = 1.0f;  // shelf slope
    float alpha = sinW / 2.0f * std::sqrt((A + 1.0f/A) * (1.0f/S - 1.0f) + 2.0f);

    float b0 =  A * ((A+1) - (A-1)*cosW + 2*std::sqrt(A)*alpha);
    float b1 =  2*A * ((A-1) - (A+1)*cosW);
    float b2 =  A * ((A+1) - (A-1)*cosW - 2*std::sqrt(A)*alpha);
    float a0 =       (A+1) + (A-1)*cosW + 2*std::sqrt(A)*alpha;
    float a1 = -2 * ((A-1) + (A+1)*cosW);
    float a2 =       (A+1) + (A-1)*cosW - 2*std::sqrt(A)*alpha;

    bq.b0 = b0/a0; bq.b1 = b1/a0; bq.b2 = b2/a0;
    bq.a1 = a1/a0; bq.a2 = a2/a0;
}

// ── High Shelf ────────────────────────────────────────────────────────────────
void FxEQ::calcHighShelf(Biquad& bq, float freqHz, float gainDB, double sr) {
    float A  = std::pow(10.0f, gainDB / 40.0f);
    float w0 = 2.0f * kPi * freqHz / static_cast<float>(sr);
    float cosW = std::cos(w0);
    float sinW = std::sin(w0);
    float S  = 1.0f;
    float alpha = sinW / 2.0f * std::sqrt((A + 1.0f/A) * (1.0f/S - 1.0f) + 2.0f);

    float b0 =  A * ((A+1) + (A-1)*cosW + 2*std::sqrt(A)*alpha);
    float b1 = -2*A * ((A-1) + (A+1)*cosW);
    float b2 =  A * ((A+1) + (A-1)*cosW - 2*std::sqrt(A)*alpha);
    float a0 =       (A+1) - (A-1)*cosW + 2*std::sqrt(A)*alpha;
    float a1 =  2 * ((A-1) - (A+1)*cosW);
    float a2 =       (A+1) - (A-1)*cosW - 2*std::sqrt(A)*alpha;

    bq.b0 = b0/a0; bq.b1 = b1/a0; bq.b2 = b2/a0;
    bq.a1 = a1/a0; bq.a2 = a2/a0;
}

// ── Bell (Peaking EQ) ─────────────────────────────────────────────────────────
void FxEQ::calcBell(Biquad& bq, float freqHz, float gainDB, float Q, double sr) {
    float A  = std::pow(10.0f, gainDB / 40.0f);
    float w0 = 2.0f * kPi * freqHz / static_cast<float>(sr);
    float alpha = std::sin(w0) / (2.0f * Q);
    float cosW  = std::cos(w0);

    float b0 =  1 + alpha * A;
    float b1 = -2 * cosW;
    float b2 =  1 - alpha * A;
    float a0 =  1 + alpha / A;
    float a1 = -2 * cosW;
    float a2 =  1 - alpha / A;

    bq.b0 = b0/a0; bq.b1 = b1/a0; bq.b2 = b2/a0;
    bq.a1 = a1/a0; bq.a2 = a2/a0;
}

// ── Obliczenie współczynników ze wszystkich parametrów ────────────────────────
void FxEQ::computeCoeffs() {
    // Low shelf: [0..1] → 40..800 Hz
    float lowHz  = 40.0f * std::pow(20.0f, params_[kLowFreq]);
    float lowDB  = (params_[kLowGain]  - 0.5f) * 24.0f;  // ±12 dB

    // Mid bell: [0..1] → 200..8000 Hz
    float midHz  = 200.0f * std::pow(40.0f, params_[kMidFreq]);
    float midDB  = (params_[kMidGain]  - 0.5f) * 24.0f;
    float midQ   = 0.3f * std::pow(8.0f / 0.3f, params_[kMidQ]);

    // High shelf: [0..1] → 2000..16000 Hz
    float highHz = 2000.0f * std::pow(8.0f, params_[kHighFreq]);
    float highDB = (params_[kHighGain] - 0.5f) * 24.0f;

    calcLowShelf (lowBQ_,  std::clamp(lowHz,  20.0f, 20000.0f),  lowDB,  sampleRate_);
    calcBell     (midBQ_,  std::clamp(midHz,  20.0f, 20000.0f),  midDB, midQ, sampleRate_);
    calcHighShelf(highBQ_, std::clamp(highHz, 20.0f, 20000.0f), highDB, sampleRate_);
}

void FxEQ::prepare(double sampleRate, int /*blockSize*/) {
    sampleRate_ = sampleRate;
    reset();
    computeCoeffs();
}

void FxEQ::reset() {
    lowBQ_.clear();
    midBQ_.clear();
    highBQ_.clear();
}

std::vector<PortInfo> FxEQ::getPorts() const {
    return {
        { "Audio In",  SignalType::Audio, false },
        { "Audio Out", SignalType::Audio, true  },
    };
}

void FxEQ::process(const float* const* inputs,
                   float* const*       outputs,
                   int                 numSamples) {
    const float* in  = inputs  ? inputs[0]  : nullptr;
    float*       out = outputs[0];

    for (int i = 0; i < numSamples; ++i) {
        float x = in ? in[i] : 0.0f;
        x = lowBQ_.tick(x);
        x = midBQ_.tick(x);
        x = highBQ_.tick(x);
        out[i] = x;
    }
}

void FxEQ::setParam(int i, float v) {
    if (i >= 0 && i < kNumParams) {
        params_[i] = std::clamp(v, 0.0f, 1.0f);
        computeCoeffs();  // przelicz filtry (wywołanie z message thread)
    }
}

} // namespace synth
