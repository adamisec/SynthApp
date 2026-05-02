#include <catch2/catch_test_macros.hpp>
#include "patch/PatchFormat.h"
#include "engine/ModuleGraph.h"
#include "modules/oscillators/OscA.h"
#include "modules/filters/FltLP.h"
#include "modules/mixers/Out.h"
#include "modules/effects/FxEQ.h"
#include <cmath>
#include <sstream>

using namespace synth;

static constexpr double SR = 96000.0;
static constexpr int    BS = 64;

// ── PatchFormat: save i load round-trip ──────────────────────────────────────
TEST_CASE("PatchFormat saves and loads modules", "[patch]") {
    ModuleGraph g1;
    auto oscA = std::make_unique<OscA>();
    oscA->setParam(OscA::kPitch,    0.6f);
    oscA->setParam(OscA::kWaveform, 0.5f);
    int oscId = g1.addModule(std::move(oscA));

    auto flt = std::make_unique<FltLP>();
    flt->setParam(FltLP::kCutoff,    0.4f);
    flt->setParam(FltLP::kResonance, 0.3f);
    int fltId = g1.addModule(std::move(flt));

    int outId = g1.addModule(std::make_unique<Out>());

    Cable c1 { oscId, 0, fltId, 0 };
    Cable c2 { fltId, 0, outId, 0 };
    g1.addCable(c1);
    g1.addCable(c2);

    std::vector<Cable> cables = { c1, c2 };

    // Zapisz do JSON
    auto j = PatchFormat::save(g1, cables);

    REQUIRE(j.contains("modules"));
    REQUIRE(j.contains("cables"));
    REQUIRE(j["modules"].size() == 3);
    REQUIRE(j["cables"].size()  == 2);

    // Wczytaj do nowego grafu
    ModuleGraph g2;
    std::vector<Cable> cables2;
    bool ok = PatchFormat::load(j, g2, cables2);

    REQUIRE(ok);
    REQUIRE(cables2.size() == 2);

    // Sprawdź czy parametry zostały przywrócone
    auto* oscLoaded = g2.getModule(0);
    REQUIRE(oscLoaded != nullptr);
    REQUIRE(std::abs(oscLoaded->getParam(OscA::kPitch)    - 0.6f) < 0.001f);
    REQUIRE(std::abs(oscLoaded->getParam(OscA::kWaveform) - 0.5f) < 0.001f);

    auto* fltLoaded = g2.getModule(1);
    REQUIRE(fltLoaded != nullptr);
    REQUIRE(std::abs(fltLoaded->getParam(FltLP::kCutoff)    - 0.4f) < 0.001f);
    REQUIRE(std::abs(fltLoaded->getParam(FltLP::kResonance) - 0.3f) < 0.001f);
}

TEST_CASE("PatchFormat save includes module type names", "[patch]") {
    ModuleGraph g;
    g.addModule(std::make_unique<OscA>());
    g.addModule(std::make_unique<FltLP>());

    auto j = PatchFormat::save(g, {});
    REQUIRE(j["modules"][0]["type"] == "OscA");
    REQUIRE(j["modules"][1]["type"] == "FltLP");
}

// ── FxEQ: neutralna odpowiedź przy gain=0dB ──────────────────────────────────
TEST_CASE("FxEQ unity gain at 0dB (all gains = 0.5)", "[eq]") {
    FxEQ eq;
    eq.prepare(SR, BS);
    // 0.5 = 0dB dla każdego pasma
    eq.setParam(FxEQ::kLowGain,  0.5f);
    eq.setParam(FxEQ::kMidGain,  0.5f);
    eq.setParam(FxEQ::kHighGain, 0.5f);

    // Sygnał 1kHz sinus
    std::vector<float> in(BS), out(BS);
    float phase = 0.0f;
    float dt    = 1000.0f / static_cast<float>(SR);
    for (int i = 0; i < BS; ++i) {
        in[i]   = std::sin(phase * 2.0f * 3.14159f);
        phase  += dt;
        if (phase >= 1.0f) phase -= 1.0f;
    }

    // Rozgrzej filtr (usuń przejściowe)
    for (int b = 0; b < 20; ++b) {
        const float* ins[1]  = { in.data() };
        float*       outs[1] = { out.data() };
        eq.process(ins, outs, BS);
    }

    // Po rozgrzaniu: amplituda wyjścia ≈ amplituda wejścia (±0.5 dB)
    float inRMS = 0, outRMS = 0;
    for (int i = 0; i < BS; ++i) {
        inRMS  += in[i]  * in[i];
        outRMS += out[i] * out[i];
    }
    inRMS  = std::sqrt(inRMS  / BS);
    outRMS = std::sqrt(outRMS / BS);

    float ratioDb = 20.0f * std::log10(outRMS / (inRMS + 1e-9f));
    REQUIRE(std::abs(ratioDb) < 0.5f);
}

TEST_CASE("FxEQ boosts low shelf when gain > 0.5", "[eq]") {
    FxEQ eq;
    eq.prepare(SR, BS);
    eq.setParam(FxEQ::kLowGain,  0.9f);  // ~+9.6 dB
    eq.setParam(FxEQ::kMidGain,  0.5f);
    eq.setParam(FxEQ::kHighGain, 0.5f);
    eq.setParam(FxEQ::kLowFreq,  0.3f);  // ~200Hz shelf

    // Sygnał 50Hz (poniżej shelf) — powinien być wzmocniony
    std::vector<float> in(BS), out(BS);
    float phase = 0.0f;
    float dt    = 50.0f / static_cast<float>(SR);
    for (int i = 0; i < BS; ++i) {
        in[i]   = std::sin(phase * 2.0f * 3.14159f);
        phase  += dt;
        if (phase >= 1.0f) phase -= 1.0f;
    }

    for (int b = 0; b < 100; ++b) {
        const float* ins[1]  = { in.data() };
        float*       outs[1] = { out.data() };
        eq.process(ins, outs, BS);
    }

    float inPeak = 0, outPeak = 0;
    for (int i = 0; i < BS; ++i) {
        inPeak  = std::max(inPeak,  std::abs(in[i]));
        outPeak = std::max(outPeak, std::abs(out[i]));
    }

    // Wyjście powinno być głośniejsze niż wejście przy boost low
    REQUIRE(outPeak > inPeak * 1.5f);
}
