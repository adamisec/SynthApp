#include <catch2/catch_test_macros.hpp>
#include "modules/envelopes/EnvADSR.h"
#include <vector>
#include <cmath>

using namespace synth;

// ── Pomocnik: uruchom kopertę przez N próbek, zbierz wyjście ─────────────────
static std::vector<float> runEnv(EnvADSR& env, bool gate, int samples) {
    // Gate przekazywany jako bufor wejściowy (port 0), nie przez setParam

    // Przygotuj "kabel" gate jako bufor
    std::vector<float> gateIn(samples, gate ? 1.0f : 0.0f);
    std::vector<float> out1(samples), out2(samples);

    float* inArr[1]  = { gateIn.data() };
    float* outArr[2] = { out1.data(), out2.data() };
    env.process(const_cast<const float**>((float**)inArr), outArr, samples);

    return out1;
}

// Zmierz czas (w próbkach) od chwili gdy wartość przekracza próg
static int samplesUntilThreshold(const std::vector<float>& buf, float threshold) {
    for (int i = 0; i < (int)buf.size(); ++i)
        if (buf[i] >= threshold) return i;
    return -1;
}

static int samplesUntilBelow(const std::vector<float>& buf, float threshold) {
    for (int i = 0; i < (int)buf.size(); ++i)
        if (buf[i] <= threshold) return i;
    return -1;
}

TEST_CASE("EnvADSR: wyjście zaczyna od 0 przy braku gate") {
    EnvADSR env;
    env.prepare(96000.0, 64);
    auto out = runEnv(env, false, 64);
    for (float s : out) REQUIRE(s < 0.001f);
}

TEST_CASE("EnvADSR: gate ON uruchamia atak") {
    EnvADSR env;
    env.prepare(96000.0, 512);
    env.setParam(EnvADSR::kAttack,  0.05f);  // ~50ms
    env.setParam(EnvADSR::kDecay,   0.3f);
    env.setParam(EnvADSR::kSustain, 0.7f);
    env.setParam(EnvADSR::kRelease, 0.3f);

    // Gate ON przez 10000 próbek
    std::vector<float> gateIn(10000, 1.0f);
    std::vector<float> out1(10000), out2(10000);

    float* inArr[1]  = { gateIn.data() };
    float* outArr[2] = { out1.data(), out2.data() };
    env.process(const_cast<const float**>((float**)inArr), outArr, 10000);

    // Powinien osiągnąć > 0.9 (szczyt ataku)
    float maxVal = *std::max_element(out1.begin(), out1.end());
    REQUIRE(maxVal >= 0.9f);

    // Powinien ustabilizować się w okolicach sustain (0.7)
    float lastVal = out1.back();
    REQUIRE(std::abs(lastVal - 0.7f) < 0.05f);
}

TEST_CASE("EnvADSR: gate OFF uruchamia release do 0") {
    EnvADSR env;
    env.prepare(96000.0, 512);
    env.setParam(EnvADSR::kAttack,  0.0f);   // minimalny atak
    env.setParam(EnvADSR::kDecay,   0.0f);
    env.setParam(EnvADSR::kSustain, 1.0f);   // sustain = 100%
    env.setParam(EnvADSR::kRelease, 0.3f);   // ~1s release

    // Faza sustain — 5000 próbek gate ON
    std::vector<float> gateOn(5000, 1.0f);
    std::vector<float> out1(5000), out2(5000);
    float* inArr[1]  = { gateOn.data() };
    float* outArr[2] = { out1.data(), out2.data() };
    env.process(const_cast<const float**>((float**)inArr), outArr, 5000);

    // Powinien być przy 1.0
    REQUIRE(out1.back() >= 0.95f);

    // Faza release — gate OFF, 50000 próbek
    std::vector<float> gateOff(50000, 0.0f);
    std::vector<float> rel1(50000), rel2(50000);
    float* inArr2[1]  = { gateOff.data() };
    float* outArr2[2] = { rel1.data(), rel2.data() };
    env.process(const_cast<const float**>((float**)inArr2), outArr2, 50000);

    // Powinien spaść poniżej 0.01
    float finalVal = rel1.back();
    REQUIRE(finalVal < 0.01f);
}

TEST_CASE("EnvADSR: wyjście zawsze w zakresie [0..1]") {
    EnvADSR env;
    env.prepare(96000.0, 64);

    // Testuj ekstremalne wartości parametrów
    env.setParam(EnvADSR::kAttack,  0.0f);
    env.setParam(EnvADSR::kDecay,   1.0f);
    env.setParam(EnvADSR::kSustain, 0.5f);
    env.setParam(EnvADSR::kRelease, 0.5f);

    // Gate ON → OFF → ON wielokrotnie
    std::vector<float> gateSeq;
    for (int i = 0; i < 10; ++i) {
        for (int j = 0; j < 2000; ++j) gateSeq.push_back(1.0f);
        for (int j = 0; j < 2000; ++j) gateSeq.push_back(0.0f);
    }

    int n = (int)gateSeq.size();
    std::vector<float> out1(n), out2(n);
    float* inArr[1]  = { gateSeq.data() };
    float* outArr[2] = { out1.data(), out2.data() };
    env.process(const_cast<const float**>((float**)inArr), outArr, n);

    for (int i = 0; i < n; ++i) {
        REQUIRE(out1[i] >= -0.001f);
        REQUIRE(out1[i] <=  1.001f);
        REQUIRE(std::isfinite(out1[i]));
    }
}

TEST_CASE("EnvADSR: inverted output = 1 - env output") {
    EnvADSR env;
    env.prepare(96000.0, 512);
    env.setParam(EnvADSR::kAttack,  0.1f);
    env.setParam(EnvADSR::kDecay,   0.2f);
    env.setParam(EnvADSR::kSustain, 0.6f);
    env.setParam(EnvADSR::kRelease, 0.3f);

    std::vector<float> gateIn(5000, 1.0f);
    std::vector<float> out1(5000), out2(5000);
    float* inArr[1]  = { gateIn.data() };
    float* outArr[2] = { out1.data(), out2.data() };
    env.process(const_cast<const float**>((float**)inArr), outArr, 5000);

    for (int i = 0; i < 5000; ++i) {
        REQUIRE(std::abs((out1[i] + out2[i]) - 1.0f) < 0.001f);
    }
}
