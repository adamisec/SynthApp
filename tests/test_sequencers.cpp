#include <catch2/catch_test_macros.hpp>
#include "modules/sequencers/SeqNote.h"
#include "modules/sequencers/SeqCtrl.h"
#include "modules/sequencers/ClkDiv.h"
#include "modules/lfos/LfoB.h"
#include "modules/envelopes/EnvFollow.h"
#include "modules/mixers/Mix8to1.h"
#include <vector>
#include <cmath>

using namespace synth;

static constexpr double SR = 96000.0;
static constexpr int    BS = 64;

// ── Pomocnik: generuj bufor N impulsów (krok=1 na 1 blok) ───────────────────
static void sendClock(SeqNote& seq, int steps, std::vector<float>& notesOut, std::vector<float>& gatesOut) {
    static const float kGateLength = 960.0f;
    int blockSamples = BS;

    for (int s = 0; s < steps; ++s) {
        std::vector<float> clkBuf(blockSamples, 0.0f);
        std::vector<float> rstBuf(blockSamples, 0.0f);
        std::vector<float> noteOut(blockSamples);
        std::vector<float> gateOut(blockSamples);

        clkBuf[0] = 1.0f;  // rising edge na początku bloku

        const float* ins[2]  = { clkBuf.data(), rstBuf.data() };
        float*       outs[2] = { noteOut.data(), gateOut.data() };
        seq.process(ins, outs, blockSamples);

        notesOut.push_back(noteOut[blockSamples / 2]);
        gatesOut.push_back(gateOut[blockSamples / 2]);
    }
}

// ── SeqNote: sekwencer wraca do kroku 1 po N krokach ─────────────────────────
TEST_CASE("SeqNote wraps after length steps", "[sequencer]") {
    SeqNote seq;
    seq.prepare(SR, BS);
    seq.setParam(SeqNote::kLength, (4.0f - 1.0f) / 15.0f);  // 4 kroki

    // Ustaw 4 różne nuty
    seq.setParam(SeqNote::kStep0, 60.0f / 127.0f);
    seq.setParam(SeqNote::kStep1, 62.0f / 127.0f);
    seq.setParam(SeqNote::kStep2, 64.0f / 127.0f);
    seq.setParam(SeqNote::kStep3, 65.0f / 127.0f);
    // Włącz gate dla wszystkich
    for (int i = 0; i < 4; ++i)
        seq.setParam(SeqNote::kGate0 + i, 0.66f);

    std::vector<float> notes, gates;
    sendClock(seq, 8, notes, gates);  // 8 kroków = 2 pełne pętle

    // Kroki 0..3 i 4..7 powinny mieć te same nuty CV (pętla)
    for (int s = 0; s < 4; ++s) {
        REQUIRE(std::abs(notes[s] - notes[s + 4]) < 0.01f);
    }
}

// ── SeqNote: reset wraca do kroku 1 ──────────────────────────────────────────
TEST_CASE("SeqNote resets on reset pulse", "[sequencer]") {
    SeqNote seq;
    seq.prepare(SR, BS);
    seq.setParam(SeqNote::kLength, (8.0f - 1.0f) / 15.0f);

    std::vector<float> noteFirst(BS), gateFirst(BS);
    std::vector<float> noteMid(BS),   gateMid(BS);
    std::vector<float> noteAfterRst(BS), gateAfterRst(BS);
    std::vector<float> clk(BS, 0.0f), rst(BS, 0.0f), empty(BS, 0.0f);

    // Krok 1 (krok 0 po init)
    clk[0] = 1.0f;
    const float* ins1[2]  = { clk.data(), empty.data() };
    float*       outs1[2] = { noteFirst.data(), gateFirst.data() };
    seq.process(ins1, outs1, BS);
    float note1 = noteFirst[BS / 2];

    // Krok 2
    const float* ins2[2]  = { clk.data(), empty.data() };
    float*       outs2[2] = { noteMid.data(), gateMid.data() };
    seq.process(ins2, outs2, BS);

    // Reset + krok — powinniśmy wrócić na krok 0
    rst[0] = 1.0f; clk[0] = 0.0f;
    const float* ins3[2]  = { clk.data(), rst.data() };
    float*       outs3[2] = { noteAfterRst.data(), gateAfterRst.data() };
    seq.process(ins3, outs3, BS);

    clk[0] = 1.0f; rst[0] = 0.0f;
    const float* ins4[2]  = { clk.data(), empty.data() };
    float*       outs4[2] = { noteAfterRst.data(), gateAfterRst.data() };
    seq.process(ins4, outs4, BS);
    float noteAfterReset = noteAfterRst[BS / 2];

    // Po resecie nuta powinna być jak pierwsza
    REQUIRE(std::abs(note1 - noteAfterReset) < 0.01f);
}

// ── SeqCtrl: wartości CV ──────────────────────────────────────────────────────
TEST_CASE("SeqCtrl outputs correct CV per step", "[sequencer]") {
    SeqCtrl seq;
    seq.prepare(SR, BS);
    seq.setParam(SeqCtrl::kLength, (4.0f - 1.0f) / 15.0f);
    seq.setParam(SeqCtrl::kSmooth, 0.0f);  // bez wygładzania

    // Ustaw 4 wartości
    seq.setParam(SeqCtrl::kStep0, 0.0f);   // -1.0 CV
    seq.setParam(SeqCtrl::kStep1, 0.5f);   //  0.0 CV
    seq.setParam(SeqCtrl::kStep2, 1.0f);   // +1.0 CV
    seq.setParam(SeqCtrl::kStep3, 0.25f);  // -0.5 CV

    // Sekwencer startuje na kroku 0. Pierwsze taktowanie przesuwa na krok 1.
    // Po 4 taktowaniach: krok 1, 2, 3, 0 (wrap)
    const float expected[] = { 0.0f, 1.0f, -0.5f, -1.0f };

    for (int s = 0; s < 4; ++s) {
        std::vector<float> cvOut(BS), gateOut(BS);
        std::vector<float> clkBuf(BS, 0.0f), rstBuf(BS, 0.0f);
        clkBuf[0] = 1.0f;

        const float* ins[2]  = { clkBuf.data(), rstBuf.data() };
        float*       outs[2] = { cvOut.data(), gateOut.data() };
        seq.process(ins, outs, BS);

        REQUIRE(std::abs(cvOut[BS / 2] - expected[s]) < 0.02f);
    }
}

// ── ClkDiv: /2 daje impuls co 2 wejściowe ────────────────────────────────────
TEST_CASE("ClkDiv divides clock by 2", "[sequencer]") {
    ClkDiv div;
    div.prepare(SR, BS);

    // Impuls trwa kPulseLength=960 sampli (~10ms). Żeby poprawnie liczyć
    // rosnące zbocza, trzeba rozdzielić clock impulsami > 960 sampli.
    // Używamy bloków 2048 sampli (>960), żeby każdy impuls zdążył wygasnąć.
    static constexpr int kSpacedBS = 2048;

    int pulseCount = 0;
    bool prevDiv2  = false;

    for (int step = 0; step < 8; ++step) {
        std::vector<float> clkBuf(kSpacedBS, 0.0f);
        std::vector<float> out2(kSpacedBS), out4(kSpacedBS),
                           out8(kSpacedBS), out16(kSpacedBS), out32(kSpacedBS);
        clkBuf[0] = 1.0f;  // jeden impuls per długi blok

        const float* ins[1]  = { clkBuf.data() };
        float*       outs[5] = { out2.data(), out4.data(), out8.data(),
                                 out16.data(), out32.data() };
        div.process(ins, outs, kSpacedBS);

        // /2 powinno dać impuls na sample 0 drugiego kroku (liczymy rising edge)
        bool div2 = out2[0] > 0.5f;
        if (div2 && !prevDiv2) ++pulseCount;
        prevDiv2 = out2[kSpacedBS - 1] > 0.5f;  // stan po całym bloku
    }

    // 8 impulsów wejścia → 4 impulsy /2
    REQUIRE(pulseCount == 4);
}

// ── LfoB: sync resetuje fazę do kStartPhase ──────────────────────────────────
TEST_CASE("LfoB sync resets phase", "[lfo]") {
    LfoB lfo;
    lfo.prepare(SR, BS);
    lfo.setParam(LfoB::kRate,       0.5f);
    lfo.setParam(LfoB::kWaveform,   0.0f);  // sine
    lfo.setParam(LfoB::kDepth,      1.0f);
    lfo.setParam(LfoB::kStartPhase, 0.0f);  // sync → faza 0 → sin(0) = 0

    // Uruchom przez kilka bloków bez synca
    for (int b = 0; b < 10; ++b) {
        std::vector<float> out(BS), empty(BS, 0.0f);
        const float* ins[2]  = { empty.data(), nullptr };
        float*       outs[1] = { out.data() };
        lfo.process(ins, outs, BS);
    }

    // Teraz sync rising edge
    std::vector<float> syncBuf(BS, 0.0f);
    std::vector<float> out(BS);
    syncBuf[0] = 1.0f;
    const float* ins[2]  = { syncBuf.data(), nullptr };
    float*       outs[1] = { out.data() };
    lfo.process(ins, outs, BS);

    // Pierwsza próbka po syncu powinna być bliska sin(0) = 0
    REQUIRE(std::abs(out[1]) < 0.1f);
}

// ── EnvFollow: śledzi amplitudę sygnału ──────────────────────────────────────
TEST_CASE("EnvFollow tracks signal amplitude", "[envelope]") {
    EnvFollow ef;
    ef.prepare(SR, BS);
    ef.setParam(EnvFollow::kAttack,  0.0f);   // szybki atak
    ef.setParam(EnvFollow::kRelease, 0.3f);
    ef.setParam(EnvFollow::kMode,    1.0f);   // peak
    ef.setParam(EnvFollow::kGain,    0.5f);

    // Cicha sekcja
    {
        std::vector<float> in(BS, 0.0f), out(BS);
        const float* ins[1]  = { in.data() };
        float*       outs[1] = { out.data() };
        for (int b = 0; b < 20; ++b)
            ef.process(ins, outs, BS);
        REQUIRE(out[BS - 1] < 0.01f);
    }

    // Głośna sekcja (pełna amplituda)
    {
        std::vector<float> in(BS, 1.0f), out(BS);
        const float* ins[1]  = { in.data() };
        float*       outs[1] = { out.data() };
        for (int b = 0; b < 50; ++b)
            ef.process(ins, outs, BS);
        // Env powinien wzrosnąć
        REQUIRE(out[BS - 1] > 0.1f);
    }
}

// ── Mix8to1: suma 8 wejść ──────────────────────────────────────────────────
TEST_CASE("Mix8to1 sums 8 inputs with levels", "[mixer]") {
    Mix8to1 mix;
    mix.prepare(SR, BS);

    // Wszystkie poziomy = 1.0
    for (int i = 0; i < 8; ++i)
        mix.setParam(i, 1.0f);

    std::vector<float> in0(BS, 0.25f);
    std::vector<float> in1(BS, 0.25f);
    std::vector<float> zeros(BS, 0.0f);
    std::vector<float> out(BS);

    const float* ins[8]  = { in0.data(), in1.data(),
                              zeros.data(), zeros.data(),
                              zeros.data(), zeros.data(),
                              zeros.data(), zeros.data() };
    float*       outs[1] = { out.data() };
    mix.process(ins, outs, BS);

    // 0.25 + 0.25 = 0.5
    REQUIRE(std::abs(out[0] - 0.5f) < 0.001f);
}

TEST_CASE("Mix8to1 respects individual levels", "[mixer]") {
    Mix8to1 mix;
    mix.prepare(SR, BS);

    // Tylko kanał 0 na pełnym poziomie
    for (int i = 0; i < 8; ++i)
        mix.setParam(i, (i == 0) ? 1.0f : 0.0f);

    std::vector<float> full(BS, 1.0f);
    std::vector<float> zeros(BS, 0.0f);
    std::vector<float> out(BS);

    const float* ins[8]  = { full.data(), full.data(),
                              full.data(), full.data(),
                              full.data(), full.data(),
                              full.data(), full.data() };
    float*       outs[1] = { out.data() };
    mix.process(ins, outs, BS);

    // Tylko kanał 0 przechodzi
    REQUIRE(std::abs(out[0] - 1.0f) < 0.001f);
}
