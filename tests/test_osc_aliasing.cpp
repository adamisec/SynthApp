#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "modules/oscillators/OscA.h"
#include <vector>
#include <cmath>
#include <numbers>
#include <algorithm>

using namespace synth;

// ── Pomocnik: wygeneruj N próbek OscA ────────────────────────────────────────
static std::vector<float> generateOscA(float midiNote, int waveform,
                                        int numSamples, double sr = 96000.0) {
    OscA osc;
    osc.prepare(sr, 64);
    osc.setParam(OscA::kPitch,    midiNote / 127.0f);
    osc.setParam(OscA::kWaveform, waveform / 4.99f);

    std::vector<float> out(numSamples);
    float* outPtr = out.data();
    const float* nullIn[2] = { nullptr, nullptr };
    float* outArr[1] = { outPtr };

    int offset = 0;
    while (offset < numSamples) {
        int block = std::min(64, numSamples - offset);
        osc.process(nullIn, outArr, block);
        outArr[0] += block;
        offset += block;
    }
    return out;
}

TEST_CASE("OscA: sine generuje DC ≈ 0") {
    auto buf = generateOscA(69.0f, 0 /* sine */, 96000);
    float sum = 0.0f;
    for (float s : buf) sum += s;
    float dc = sum / buf.size();
    REQUIRE(std::abs(dc) < 0.001f);
}

TEST_CASE("OscA: sine ma amplitudę w zakresie ±1") {
    auto buf = generateOscA(69.0f, 0, 4096);
    float maxAbs = 0.0f;
    for (float s : buf) maxAbs = std::max(maxAbs, std::abs(s));
    REQUIRE(maxAbs <= 1.01f);
    REQUIRE(maxAbs >= 0.99f);  // Musi być blisko 1
}

TEST_CASE("OscA: sawtooth DC ≈ 0 (PolyBLEP)") {
    auto buf = generateOscA(69.0f, 2 /* saw */, 96000);
    float sum = 0.0f;
    for (float s : buf) sum += s;
    float dc = sum / buf.size();
    REQUIRE(std::abs(dc) < 0.01f);
}

TEST_CASE("OscA: square DC ≈ 0 przy 50% PWM") {
    auto buf = generateOscA(60.0f, 3 /* square */, 96000);
    float sum = 0.0f;
    for (float s : buf) sum += s;
    float dc = sum / buf.size();
    REQUIRE(std::abs(dc) < 0.01f);
}

TEST_CASE("OscA: próbki mieszczą się w ±1.1 (brak przepełnienia)") {
    // Sprawdź wszystkie kształty przy kilku nutach
    for (int wave = 0; wave <= 4; ++wave) {
        for (float note : { 24.0f, 60.0f, 96.0f, 120.0f }) {
            auto buf = generateOscA(note, wave, 2048);
            for (float s : buf) {
                REQUIRE(std::abs(s) <= 1.1f);
            }
        }
    }
}

TEST_CASE("OscA: częstotliwość A4 (440 Hz) przy midi=69") {
    // Zmierz period przez zero-crossing
    auto buf = generateOscA(69.0f, 0 /* sine */, 96000 * 2);
    double sr = 96000.0;

    int crossings = 0;
    int firstCross = -1;
    int lastCross  = -1;

    for (int i = 1; i < (int)buf.size(); ++i) {
        if (buf[i-1] <= 0.0f && buf[i] > 0.0f) {
            if (firstCross < 0) firstCross = i;
            lastCross = i;
            crossings++;
        }
    }

    REQUIRE(crossings >= 2);
    double measuredHz = static_cast<double>(crossings - 1) * sr
                      / (lastCross - firstCross);

    // Tolerancja 2 Hz
    REQUIRE(std::abs(measuredHz - 440.0) < 2.0);
}

TEST_CASE("OscA: reset() zeruje fazę") {
    OscA osc;
    osc.prepare(96000.0, 64);
    osc.setParam(OscA::kPitch,    69.0f / 127.0f);
    osc.setParam(OscA::kWaveform, 0.0f);  // sine

    std::vector<float> out1(64), out2(64);
    const float* nullIn[2] = {};
    float* arr1[1] = { out1.data() };
    float* arr2[1] = { out2.data() };

    osc.process(nullIn, arr1, 64);
    osc.reset();
    osc.process(nullIn, arr2, 64);

    // Po resecie oba przebiegi powinny być identyczne
    for (int i = 0; i < 64; ++i) {
        REQUIRE(out1[i] == Catch::Approx(out2[i]).margin(1e-6f));
    }
}
