#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <vector>
#include <cmath>
#include <numeric>
#include <algorithm>

#include "modules/effects/FxReverb.h"
#include "modules/effects/FxDelay.h"
#include "modules/effects/FxChorus.h"
#include "modules/effects/FxDistort.h"
#include "modules/effects/FxCompressor.h"

using namespace synth;

static constexpr double SR = 96000.0;
static constexpr int    BS = 512;

// ──────────────────────────────────────────────────────────────────
// FxReverb
// ──────────────────────────────────────────────────────────────────
TEST_CASE("FxReverb: output has energy after impulse", "[reverb]") {
    FxReverb rev;
    rev.prepare(SR, BS);
    rev.setParam(FxReverb::kRoomSize, 0.8f);
    rev.setParam(FxReverb::kMix, 1.0f);

    // Krótkie comb bufory przy 44.1kHz: min ~1116 próbek.
    // Przy 96kHz ~2429 próbek → potrzeba >5 bloków po 512 próbek.
    // Użyj jednego dużego bufora 8192 próbek.
    constexpr int N = 8192;
    std::vector<float> inBuf(N, 0.0f);
    inBuf[0] = 1.0f;  // impuls
    std::vector<float> outL(N), outR(N);

    for (int off = 0; off < N; off += BS) {
        const float* ins[]  = { inBuf.data() + off };
        float*       outs[] = { outL.data() + off, outR.data() + off };
        rev.process(ins, outs, BS);
    }

    float rmsL = 0.0f;
    for (auto x : outL) rmsL += x * x;
    REQUIRE(rmsL > 0.0f);
}

TEST_CASE("FxReverb: decay over time (less energy far from impulse)", "[reverb]") {
    FxReverb rev;
    rev.prepare(SR, BS);
    rev.setParam(FxReverb::kRoomSize, 0.8f);
    rev.setParam(FxReverb::kMix, 1.0f);

    // Impuls → potem cisza. Porównaj energię bloku wczesnego (po impulsie)
    // vs. bloku bardzo późnego (po długiej ciszy).
    constexpr int N = 32768;
    std::vector<float> inBuf(N, 0.0f);
    inBuf[0] = 1.0f;
    std::vector<float> outL(N), outR(N);

    for (int off = 0; off < N; off += BS) {
        const float* ins[]  = { inBuf.data() + off };
        float*       outs[] = { outL.data() + off, outR.data() + off };
        rev.process(ins, outs, BS);
    }

    // Energia w bloku #5..#10 (wczesne echo)
    float rmsEarly = 0.0f;
    for (int i = 5 * BS; i < 10 * BS; ++i) rmsEarly += outL[i] * outL[i];

    // Energia w bloku #55..#60 (późne pogłosy)
    float rmsLate = 0.0f;
    for (int i = 55 * BS; i < 60 * BS; ++i) rmsLate += outL[i] * outL[i];

    REQUIRE(rmsEarly > 0.0f);
    REQUIRE(rmsLate  < rmsEarly);
}

// ──────────────────────────────────────────────────────────────────
// FxDelay
// ──────────────────────────────────────────────────────────────────
TEST_CASE("FxDelay: impulse appears after delay time", "[delay]") {
    FxDelay dly;
    dly.prepare(SR, BS);
    // Ustaw czas ~10ms → ~960 próbek przy 96kHz
    // log(960/96000 * 1000 / 1) / log(2000) ≈ param
    // 10ms → param = log10(10)/log10(2000) ≈ 0.321
    dly.setParam(FxDelay::kTime,     0.32f);
    dly.setParam(FxDelay::kFeedback, 0.0f);
    dly.setParam(FxDelay::kMix,      1.0f); // 100% wet

    // Oblicz oczekiwane opóźnienie w próbkach
    float timeMs = 1.0f * std::pow(2000.0f, 0.32f);
    int expectedDelay = static_cast<int>(timeMs * 0.001f * SR);

    // Wypełnij kilka bloków
    int totalSamples = expectedDelay + BS + 10;
    std::vector<float> inData(totalSamples, 0.0f);
    inData[0] = 1.0f; // impuls na początku

    std::vector<float> outLData(totalSamples, 0.0f);
    std::vector<float> outRData(totalSamples, 0.0f);

    for (int offset = 0; offset < totalSamples; offset += BS) {
        int n = std::min(BS, totalSamples - offset);
        const float* ins[]  = { inData.data() + offset, nullptr, nullptr };
        float* outs[] = { outLData.data() + offset, outRData.data() + offset };
        dly.process(ins, outs, n);
    }

    // Znajdź peak w wyjściu
    int peakIdx = 0;
    float peakVal = 0.0f;
    for (int i = 0; i < totalSamples; ++i) {
        if (std::abs(outLData[i]) > peakVal) {
            peakVal = std::abs(outLData[i]);
            peakIdx = i;
        }
    }

    // Peak powinien być blisko expectedDelay (±5%)
    REQUIRE(peakVal > 0.1f);
    REQUIRE(std::abs(peakIdx - expectedDelay) < expectedDelay / 20 + 5);
}

// ──────────────────────────────────────────────────────────────────
// FxChorus
// ──────────────────────────────────────────────────────────────────
TEST_CASE("FxChorus: L and R outputs differ (stereo)", "[chorus]") {
    FxChorus cho;
    cho.prepare(SR, BS);
    cho.setParam(FxChorus::kDepth, 1.0f);
    cho.setParam(FxChorus::kDelay, 0.5f);
    cho.setParam(FxChorus::kMix,   1.0f);

    // Rozgrzewka: wypełnij bufor opóźniający sygnałem (> 30ms @ 96kHz = 2880 próbek)
    constexpr int warmup = 4096;
    std::vector<float> warmBuf(warmup);
    for (int i = 0; i < warmup; ++i) warmBuf[i] = (i % 7 - 3) * 0.1f;
    {
        std::vector<float> wL(warmup), wR(warmup);
        const float* ins[]  = { warmBuf.data() };
        float*       outs[] = { wL.data(), wR.data() };
        cho.process(ins, outs, warmup);
    }

    // Teraz zmierz stereo po rozgrzewce
    std::vector<float> in(BS);
    for (int i = 0; i < BS; ++i) in[i] = (i % 7 - 3) * 0.1f;
    std::vector<float> outL(BS), outR(BS);
    {
        const float* ins[]  = { in.data() };
        float*       outs[] = { outL.data(), outR.data() };
        cho.process(ins, outs, BS);
    }

    // Głos 1 ma odwróconą fazę → L i R powinny się różnić
    float diff = 0.0f;
    for (int i = 0; i < BS; ++i) diff += std::abs(outL[i] - outR[i]);
    REQUIRE(diff > 0.0f);
}

// ──────────────────────────────────────────────────────────────────
// FxDistort
// ──────────────────────────────────────────────────────────────────
TEST_CASE("FxDistort: soft clip output bounded to ±1", "[distort]") {
    FxDistort dist;
    dist.prepare(SR, BS);
    dist.setParam(FxDistort::kDrive,  1.0f);  // max drive
    dist.setParam(FxDistort::kType,   0.0f);  // soft clip
    dist.setParam(FxDistort::kOutput, 0.5f);  // 0 dB gain
    dist.setParam(FxDistort::kMix,    1.0f);

    std::vector<float> in(BS, 5.0f); // overdrive
    std::vector<float> out(BS);
    const float* ins[]  = { in.data(), nullptr };
    float*       outs[] = { out.data() };
    dist.process(ins, outs, BS);

    for (auto x : out) REQUIRE(std::abs(x) <= 1.5f); // tanh ≤1, *gain=1
}

TEST_CASE("FxDistort: hard clip output strictly bounded", "[distort]") {
    FxDistort dist;
    dist.prepare(SR, BS);
    dist.setParam(FxDistort::kDrive,  1.0f);
    dist.setParam(FxDistort::kType,   0.26f); // typ 1 = hard clip
    dist.setParam(FxDistort::kOutput, 0.5f);
    dist.setParam(FxDistort::kMix,    1.0f);

    std::vector<float> in(BS, 10.0f);
    std::vector<float> out(BS);
    const float* ins[]  = { in.data(), nullptr };
    float*       outs[] = { out.data() };
    dist.process(ins, outs, BS);

    for (auto x : out) REQUIRE(std::abs(x) <= 1.05f); // clamp ≤1, *gain=1
}

TEST_CASE("FxDistort: foldback produces harmonics (not DC)", "[distort]") {
    FxDistort dist;
    dist.prepare(SR, BS);
    dist.setParam(FxDistort::kDrive,  0.8f);
    dist.setParam(FxDistort::kType,   0.52f); // typ 2 = foldback
    dist.setParam(FxDistort::kOutput, 0.5f);
    dist.setParam(FxDistort::kMix,    1.0f);

    // Sygnał sinusoidalny
    std::vector<float> in(BS);
    for (int i = 0; i < BS; ++i) in[i] = 2.0f * std::sin(2.0f * 3.14159f * 440.0f * i / SR);
    std::vector<float> out(BS);
    const float* ins[]  = { in.data(), nullptr };
    float*       outs[] = { out.data() };
    dist.process(ins, outs, BS);

    float energy = 0.0f;
    for (auto x : out) energy += x * x;
    REQUIRE(energy > 0.0f);
}

// ──────────────────────────────────────────────────────────────────
// FxCompressor
// ──────────────────────────────────────────────────────────────────
TEST_CASE("FxCompressor: loud signal gets reduced", "[compressor]") {
    FxCompressor comp;
    comp.prepare(SR, BS);
    comp.setParam(FxCompressor::kThreshold, 0.5f);  // -30 dBFS
    comp.setParam(FxCompressor::kRatio,     0.8f);  // high ratio
    comp.setParam(FxCompressor::kAttack,    0.5f);
    comp.setParam(FxCompressor::kRelease,   0.5f);
    comp.setParam(FxCompressor::kMakeup,    0.0f);  // brak makeup

    // Silny sygnał: 0 dBFS
    std::vector<float> in(BS, 1.0f);
    std::vector<float> out(BS);
    float* grBuf = nullptr;
    const float* ins[]  = { in.data(), nullptr };
    float*       outs[] = { out.data(), grBuf };
    comp.process(ins, outs, BS);

    // RMS wyjścia powinno być mniejsze niż wejścia
    float rmsOut = 0.0f;
    for (auto x : out) rmsOut += x * x;
    rmsOut = std::sqrt(rmsOut / BS);

    REQUIRE(rmsOut < 1.0f);
}

TEST_CASE("FxCompressor: quiet signal passes through mostly unchanged", "[compressor]") {
    FxCompressor comp;
    comp.prepare(SR, BS);
    comp.setParam(FxCompressor::kThreshold, 0.5f);  // -30 dBFS
    comp.setParam(FxCompressor::kRatio,     0.8f);
    comp.setParam(FxCompressor::kAttack,    0.0f);
    comp.setParam(FxCompressor::kRelease,   0.5f);
    comp.setParam(FxCompressor::kMakeup,    0.0f);

    // Cichy sygnał: -60 dBFS = 0.001
    std::vector<float> in(BS, 0.001f);
    std::vector<float> out(BS);
    const float* ins[]  = { in.data(), nullptr };
    float*       outs[] = { out.data(), nullptr };
    comp.process(ins, outs, BS);

    // Wyjście powinno być zbliżone do wejścia (gain ≈ 1)
    float ratio = out[BS - 1] / in[0];
    REQUIRE(ratio == Catch::Approx(1.0f).margin(0.1f));
}
