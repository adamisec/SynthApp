#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "modules/filters/FltLP.h"
#include <vector>
#include <cmath>
#include <numbers>

using namespace synth;

// ── Generuj sinus o danej częstotliwości ─────────────────────────────────────
static std::vector<float> makeSine(double freq, double sr, int samples) {
    std::vector<float> buf(samples);
    for (int i = 0; i < samples; ++i)
        buf[i] = std::sin(2.0 * std::numbers::pi * freq / sr * i);
    return buf;
}

// ── Zmierz RMS sygnału ────────────────────────────────────────────────────────
static float rms(const float* buf, int n) {
    float sum = 0.0f;
    for (int i = 0; i < n; ++i) sum += buf[i] * buf[i];
    return std::sqrt(sum / n);
}

// ── Przepuść sygnał przez filtr, zmierz RMS na wyjściu ───────────────────────
// Przetwarzamy sygnał ciągle (warmup + pomiar bez przerwy), żeby uniknąć
// nieciągłości fazy między warmupem a pomiarem.
static float measureFilterOutput(FltLP& flt, const std::vector<float>& input,
                                  int warmupSamples = 4096) {
    int n = (int)input.size();

    // Sygnał rozgrzewkowy: powtórzenie input (sygnał jest periodyczny)
    std::vector<float> longSig;
    longSig.reserve(warmupSamples + n);
    for (int i = 0; i < warmupSamples; ++i)
        longSig.push_back(input[i % n]);
    longSig.insert(longSig.end(), input.begin(), input.end());

    int totalN = (int)longSig.size();
    std::vector<float> outLong(totalN), outLong2(totalN);

    float* inArr[3]  = { longSig.data(), nullptr, nullptr };
    float* outArr[2] = { outLong.data(), outLong2.data() };

    flt.process(const_cast<const float**>((float**)inArr), outArr, totalN);

    // Pomiar tylko z drugiej połowy (po warmupie)
    return rms(outLong.data() + warmupSamples, n);
}

TEST_CASE("FltLP: przepuszcza niskie częstotliwości, tłumi wysokie") {
    FltLP flt;
    flt.prepare(96000.0, 1024);
    flt.setParam(FltLP::kCutoff,    0.5f);   // ~1 kHz
    flt.setParam(FltLP::kResonance, 0.0f);
    flt.setParam(FltLP::kInputGain, 0.5f);

    auto low  = makeSine(100.0,  96000.0, 4096);
    auto high = makeSine(10000.0, 96000.0, 4096);

    flt.reset();
    float rmsLow  = measureFilterOutput(flt, low);

    flt.reset();
    float rmsHigh = measureFilterOutput(flt, high);

    // Niskie częstotliwości powinny przechodzić, wysokie być tłumione
    INFO("RMS low: " << rmsLow << ", RMS high: " << rmsHigh);
    REQUIRE(rmsLow > rmsHigh * 5.0f);  // minimum 14 dB różnicy
}

TEST_CASE("FltLP: tłumienie wzrasta po -24 dB/okt (4-pole)") {
    FltLP flt;
    flt.prepare(96000.0, 2048);
    flt.setParam(FltLP::kCutoff,    0.5f);  // ~1 kHz
    flt.setParam(FltLP::kResonance, 0.0f);
    flt.setParam(FltLP::kInputGain, 0.5f);

    // f1 = 2*fc, f2 = 4*fc  → tłumienie f2 powinno być ~24 dB większe niż f1
    auto sig1 = makeSine(2000.0, 96000.0, 4096);
    auto sig2 = makeSine(4000.0, 96000.0, 4096);

    // Ciągły sygnał 8192 próbek (bez sztucznego sklejania bufora — brak skoku fazy)
    int N = 4096;
    auto sig1_long = makeSine(2000.0, 96000.0, N * 2);
    auto sig2_long = makeSine(4000.0, 96000.0, N * 2);

    std::vector<float> o1(N*2), o2(N*2);
    float* ins[3]  = { sig1_long.data(), nullptr, nullptr };
    float* outs[2] = { o1.data(), o2.data() };
    flt.reset();
    flt.process(const_cast<const float**>((float**)ins), outs, N*2);
    float r1 = rms(o1.data() + N, N);

    ins[0] = sig2_long.data();
    flt.reset();
    flt.process(const_cast<const float**>((float**)ins), outs, N*2);
    float r2 = rms(o1.data() + N, N);

    float dbDiff = 20.0f * std::log10(r1 / (r2 + 1e-10f));
    INFO("dB difference per octave: " << dbDiff);

    // 4-pole = ~24 dB/okt, oczekujemy > 18 dB różnicy (margines)
    REQUIRE(dbDiff > 18.0f);
}

TEST_CASE("FltLP: brak DC offset przy zerowym wejściu") {
    FltLP flt;
    flt.prepare(96000.0, 64);
    flt.setParam(FltLP::kCutoff,    0.7f);
    flt.setParam(FltLP::kResonance, 0.5f);
    flt.setParam(FltLP::kInputGain, 0.5f);

    std::vector<float> zeros(1024, 0.0f);
    float r = measureFilterOutput(flt, zeros);
    REQUIRE(r < 0.001f);
}

TEST_CASE("FltLP: rezonans nie powoduje przepełnienia przy sinusie 1 kHz") {
    FltLP flt;
    flt.prepare(96000.0, 64);
    flt.setParam(FltLP::kCutoff,    0.5f);
    flt.setParam(FltLP::kResonance, 0.9f);  // wysokie Q
    flt.setParam(FltLP::kInputGain, 0.3f);  // mniejszy sygnał wejściowy

    auto input = makeSine(1000.0, 96000.0, 8192);
    std::vector<float> out1(8192), out2(8192);

    float* inArr[3]  = { input.data(), nullptr, nullptr };
    float* outArr[2] = { out1.data(), out2.data() };

    flt.process(const_cast<const float**>((float**)inArr), outArr, 8192);

    for (int i = 0; i < 8192; ++i) {
        REQUIRE(std::abs(out1[i]) < 10.0f);  // bez runaway
        REQUIRE(std::isfinite(out1[i]));      // brak NaN/Inf
    }
}
