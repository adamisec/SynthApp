#include "PresetManager.h"

// ── Pomocnik do budowania presetów ───────────────────────────────────────────
namespace {

// Kolejność parametrów musi byc identyczna z SimplePanelView (17 pozycji):
//  OSCYLATORY: (2,1) (1,2) (3,0) (3,1)
//  FILTR:      (4,0) (4,1) (5,2)
//  KOPERTA:    (6,0) (6,1) (6,2) (6,3)
//  LFO:        (5,0) (5,1)
//  EFEKTY:     (8,3) (9,3) (9,0) (10,0)

struct P { int mod, param; };

static const P kParamOrder[] = {
    {2,1}, {1,2}, {3,0}, {3,1},   // OSCYLATORY
    {4,0}, {4,1}, {5,2},           // FILTR
    {6,0}, {6,1}, {6,2}, {6,3},   // KOPERTA
    {5,0}, {5,1},                  // LFO
    {8,3}, {9,3}, {9,0}, {10,0},  // EFEKTY
};
static constexpr int kNumParams = 17;

PresetManager::Preset makePreset(const char* name, const float vals[kNumParams]) {
    PresetManager::Preset p;
    p.name      = name;
    p.isFactory = true;
    for (int i = 0; i < kNumParams; ++i)
        p.params.push_back({ kParamOrder[i].mod, kParamOrder[i].param, vals[i] });
    return p;
}

} // anonymous namespace

// ── Pomocnik: preset z sekwencerem Morodera ───────────────────────────────────
namespace {

PresetManager::Preset makeMoroderPreset(
    const char* name,
    const float vals[kNumParams],
    float bpm, int div, int steps,
    std::initializer_list<PresetManager::SeqStep> seq)
{
    auto p = makePreset(name, vals);
    p.hasSeq     = true;
    p.seqBPM     = bpm;
    p.seqDiv     = div;
    p.seqSteps   = steps;
    p.seqPattern = seq;
    return p;
}

} // namespace

// ── Fabryczne presety ─────────────────────────────────────────────────────────

void PresetManager::buildFactoryPresets() {
    // DETUNE  KSZTALT  POZIOMAL POZIOMB  CUTOFF  REZO   LFOMOD
    // ATAK    DECAY    SUSTAIN  RELEASE  SZYBK   LFOKT  CHORUS  REVERB  ROZM   GLOS

    static const float kSpacePad[kNumParams] = {
        0.555f, 0.50f, 0.70f, 0.70f,   // OSC
        0.45f,  0.35f, 0.22f,           // FLT
        0.25f,  0.30f, 0.75f, 0.55f,   // ENV
        0.20f,  0.00f,                  // LFO
        0.50f,  0.45f, 0.80f, 0.95f,   // FX
    };
    static const float kBrightLead[kNumParams] = {
        0.50f,  0.75f, 0.85f, 0.30f,   // OSC — square, bez detune
        0.78f,  0.55f, 0.05f,           // FLT — otwarty, sredni rezonans
        0.04f,  0.18f, 0.60f, 0.18f,   // ENV — szybki atak
        0.45f,  0.00f,                  // LFO — szybszy
        0.20f,  0.20f, 0.45f, 0.95f,   // FX — suchy
    };
    static const float kBassPluck[kNumParams] = {
        0.52f,  0.50f, 0.90f, 0.60f,   // OSC — piła
        0.28f,  0.65f, 0.00f,           // FLT — zamkniety, wysoki Q
        0.00f,  0.22f, 0.10f, 0.14f,   // ENV — bez ataku, krótki decay
        0.10f,  0.00f,                  // LFO — wolny
        0.08f,  0.22f, 0.38f, 0.90f,   // FX — mało efektów
    };
    static const float kWarmStrings[kNumParams] = {
        0.560f, 0.20f, 0.75f, 0.75f,   // OSC — trojkat, duzy detune
        0.32f,  0.12f, 0.18f,           // FLT — ciepły, malo rezonansu
        0.42f,  0.28f, 0.82f, 0.68f,   // ENV — wolny atak pad
        0.15f,  0.00f,                  // LFO — wolny sinus
        0.70f,  0.52f, 0.72f, 0.85f,   // FX — duzo chorusa i reverbu
    };
    static const float kDeepDrone[kNumParams] = {
        0.57f,  0.50f, 0.62f, 0.62f,   // OSC — piła, mocny detune
        0.22f,  0.50f, 0.40f,           // FLT — zamkniety, modulowany
        0.75f,  0.35f, 0.92f, 0.82f,   // ENV — bardzo wolny atak
        0.10f,  0.00f,                  // LFO — bardzo wolny
        0.62f,  0.80f, 0.95f, 0.80f,   // FX — maksymalny reverb
    };
    static const float kPluckHarp[kNumParams] = {
        0.50f,  0.00f, 0.80f, 0.50f,   // OSC — sinus, bez detune
        0.60f,  0.30f, 0.00f,           // FLT — jasny
        0.00f,  0.30f, 0.05f, 0.25f,   // ENV — perkusyjny
        0.30f,  0.40f,                  // LFO — square
        0.15f,  0.55f, 0.65f, 0.90f,   // FX — duzy reverb
    };
    static const float kOrgan[kNumParams] = {
        0.51f,  0.50f, 0.80f, 0.80f,   // OSC — piły równo
        0.85f,  0.10f, 0.00f,           // FLT — otwarty filtr
        0.00f,  0.00f, 0.90f, 0.05f,   // ENV — natychmiastowy
        0.35f,  0.00f,                  // LFO
        0.40f,  0.30f, 0.55f, 0.90f,   // FX
    };
    static const float kSweepPad[kNumParams] = {
        0.555f, 0.50f, 0.70f, 0.70f,   // OSC
        0.10f,  0.45f, 0.55f,           // FLT — mocne LFO na cutoff
        0.35f,  0.30f, 0.80f, 0.70f,   // ENV
        0.12f,  0.00f,                  // LFO — wolny sweep
        0.55f,  0.60f, 0.88f, 0.90f,   // FX — przestrzenny
    };

    presets_.push_back(makePreset("Space Pad",    kSpacePad));
    presets_.push_back(makePreset("Bright Lead",  kBrightLead));
    presets_.push_back(makePreset("Bass Pluck",   kBassPluck));
    presets_.push_back(makePreset("Warm Strings", kWarmStrings));
    presets_.push_back(makePreset("Deep Drone",   kDeepDrone));
    presets_.push_back(makePreset("Pluck Harp",   kPluckHarp));
    presets_.push_back(makePreset("Organ",        kOrgan));
    presets_.push_back(makePreset("Sweep Pad",    kSweepPad));

    // ── Presety Giorgio Moroder ───────────────────────────────────────────────
    //
    // Brzmienie: rezonansowy filtr drabinkowy Moog, piła, szybki atak/decay,
    // lekki chorus dla szerokości stereo (oryginał nagrywany na 24-ścieżki).
    //
    // Dźwięki MIDI: oktawa 2 = nuty 36-47 (C2=36, D2=38, Eb2=39, F2=41,
    //               G2=43), oktawa 1 = G1=31, Bb1=34, Ab1=32

    // ── "I Feel Love" — Donna Summer / Moroder (1977) ─────────────────────
    // Klasyczna pętla: C2 C2 C2 C2 G1 G1 Bb1 Bb1, 126 BPM, ósemki
    // Moog Modular + AMS delay (tu: 8 kroków + lekki chorus zamiast delay)
    static const float kIFeelLove[kNumParams] = {
        0.52f, 0.50f, 0.88f, 0.55f,   // OSC: piła, lekki detune
        0.32f, 0.72f, 0.06f,           // FLT: zamknięty, wysoki rezonans Moog
        0.00f, 0.22f, 0.18f, 0.12f,   // ENV: natychmiastowy atak, krótki decay
        0.15f, 0.00f,                  // LFO: wolny sinus
        0.42f, 0.12f, 0.40f, 0.92f,   // FX: medium chorus, mało reverbu
    };
    presets_.push_back(makeMoroderPreset(
        "I Feel Love (Moroder 77)",
        kIFeelLove, 126.0f, 8, 8,
        {
            {36,true},{36,true},{36,true},{36,true},   // C2 C2 C2 C2
            {31,true},{31,true},{34,true},{34,true},   // G1 G1 Bb1 Bb1
        }
    ));

    // ── "The Chase" — Midnight Express (Moroder 1978) ──────────────────────
    // Pętla w Gm: G2 G2 G2 G2 D2 D2 F2 F2, 125 BPM
    // Minimoog + Roland SH-2000 + MXR Flanger
    static const float kTheChase[kNumParams] = {
        0.52f, 0.50f, 0.90f, 0.50f,   // OSC: piła, minimalne OscB
        0.30f, 0.68f, 0.10f,           // FLT: ciemny, mocny rezonans
        0.00f, 0.20f, 0.15f, 0.10f,   // ENV: natychmiastowy, energiczny
        0.18f, 0.00f,                  // LFO: bardzo wolny
        0.35f, 0.18f, 0.45f, 0.90f,   // FX: wyraźny chorus (flanger)
    };
    presets_.push_back(makeMoroderPreset(
        "The Chase (Moroder 78)",
        kTheChase, 125.0f, 8, 8,
        {
            {43,true},{43,true},{43,true},{43,true},   // G2 G2 G2 G2
            {38,true},{38,true},{41,true},{41,true},   // D2 D2 F2 F2
        }
    ));

    // ── "E=MC²" — Giorgio Moroder (1979) ────────────────────────────────────
    // Wznoszący motyw 4-nutowy: C2 Eb2 F2 G2, powtórzony × 2, 124 BPM
    // Moog + Roland MC-8 MicroComposer (pierwszy cyfrowy sekwencer Morodera)
    static const float kEMC2[kNumParams] = {
        0.51f, 0.50f, 0.85f, 0.60f,   // OSC: piła, bardzo lekki detune
        0.40f, 0.60f, 0.08f,           // FLT: jaśniejszy niż IFL, wyraźna góra
        0.00f, 0.18f, 0.25f, 0.14f,   // ENV: szybki, z lekkim sustainem
        0.22f, 0.00f,                  // LFO
        0.30f, 0.20f, 0.50f, 0.90f,   // FX
    };
    presets_.push_back(makeMoroderPreset(
        "E=MC2 (Moroder 79)",
        kEMC2, 124.0f, 16, 8,
        {
            {36,true},{39,true},{41,true},{43,true},   // C2 Eb2 F2 G2
            {36,true},{39,true},{41,true},{43,true},   // powtórzenie
        }
    ));

    // ── "Together In Electric Dreams" — Phil Oakey & Moroder (1984) ────────
    // 16 kroków, Eb major: Eb2 Bb1 Ab1 Bb1, Roland Jupiter-8 + LinnDrum
    static const float kElectricDreams[kNumParams] = {
        0.53f, 0.50f, 0.75f, 0.75f,   // OSC: piły, lekki detune (Jupiter-8 unison)
        0.55f, 0.45f, 0.14f,           // FLT: otwarty, środkowy rezonans, lekkie LFO
        0.04f, 0.25f, 0.55f, 0.22f,   // ENV: delikatny atak, duży sustain
        0.18f, 0.00f,                  // LFO: wolny sinus
        0.65f, 0.35f, 0.55f, 0.90f,   // FX: dużo chorusa (typowe lata 80.)
    };
    presets_.push_back(makeMoroderPreset(
        "Electric Dreams (Moroder 84)",
        kElectricDreams, 130.0f, 16, 16,
        {
            {39,true},{39,true},{39,true},{39,true},   // Eb2 ×4
            {34,true},{34,true},{34,true},{34,true},   // Bb1 ×4
            {32,true},{32,true},{34,true},{34,true},   // Ab1 Ab1 Bb1 Bb1
            {39,true},{39,true},{39,true},{39,true},   // Eb2 ×4
        }
    ));

    // ── Presety Kraftwerk ─────────────────────────────────────────────────────
    //
    // Brzmienie: czysty Minimoog, lekko otwarty filtr, minimalne efekty —
    // Kraftwerk celowo unikał "mięsistego" brzmienia na rzecz sterylnej precyzji.

    // ── "Autobahn" (1974) ─────────────────────────────────────────────────────
    // Wznoszące triady F-dur i G-dur, 120 BPM, 16 kroków ósemkowych
    static const float kAutobahn[kNumParams] = {
        0.50f, 0.50f, 0.88f, 0.40f,   // OSC: piła, bez detune
        0.58f, 0.42f, 0.05f,           // FLT: jasny, umiarkowany rezonans
        0.00f, 0.20f, 0.45f, 0.15f,   // ENV: szybki, średni sustain
        0.15f, 0.00f,                  // LFO: wolny sinus
        0.20f, 0.15f, 0.40f, 0.90f,   // FX: suche
    };
    presets_.push_back(makeMoroderPreset(
        "Autobahn (Kraftwerk 74)",
        kAutobahn, 120.0f, 8, 16,
        {
            {41,true},{45,true},{48,true},{53,true},   // F2 A2 C3 F3
            {41,true},{45,true},{48,true},{53,true},   // F2 A2 C3 F3
            {43,true},{46,true},{50,true},{55,true},   // G2 Bb2 D3 G3
            {43,true},{46,true},{50,true},{55,true},   // G2 Bb2 D3 G3
        }
    ));

    // ── "Trans-Europe Express" (1977) ──────────────────────────────────────────
    // Pulsujące A2/E2 z chromatycznym zejściem, 116 BPM
    static const float kTEE[kNumParams] = {
        0.50f, 0.50f, 0.90f, 0.30f,   // OSC: czysta piła
        0.52f, 0.48f, 0.04f,           // FLT: lekko zamknięty
        0.00f, 0.18f, 0.38f, 0.12f,   // ENV: szybki, mechaniczny
        0.12f, 0.00f,
        0.15f, 0.10f, 0.35f, 0.90f,
    };
    presets_.push_back(makeMoroderPreset(
        "Trans-Europe Exp (Kraftwerk 77)",
        kTEE, 116.0f, 8, 16,
        {
            {45,true},{45,true},{40,true},{45,true},   // A2 A2 E2 A2
            {45,true},{40,true},{45,true},{43,true},   // A2 E2 A2 G2
            {45,true},{45,true},{40,true},{45,true},   // A2 A2 E2 A2
            {45,true},{40,true},{41,true},{40,true},   // A2 E2 F2 E2
        }
    ));

    // ── "The Model" (1978) ────────────────────────────────────────────────────
    // A-mol z pauzami, 126 BPM — pauzy zakodowane jako inactive steps
    static const float kTheModel[kNumParams] = {
        0.50f, 0.50f, 0.90f, 0.25f,   // OSC
        0.55f, 0.48f, 0.03f,           // FLT
        0.00f, 0.16f, 0.30f, 0.12f,   // ENV: ostry, robotyczny
        0.12f, 0.00f,
        0.15f, 0.10f, 0.35f, 0.90f,
    };
    presets_.push_back(makeMoroderPreset(
        "The Model (Kraftwerk 78)",
        kTheModel, 126.0f, 8, 16,
        {
            {45,true },{45,false},{45,true },{48,true },   // A2 — A2 C3
            {52,true },{52,false},{52,true },{50,true },   // E3 — E3 D3
            {45,true },{45,false},{45,true },{48,true },   // A2 — A2 C3
            {52,true },{53,true },{52,true },{52,false},   // E3 F3 E3 —
        }
    ));

    // ── "Computer Love" (1981) ────────────────────────────────────────────────
    // Melodia F-dur (synth lead), 128 BPM — cieplejszy ton niż klasyczny Kraft
    static const float kComputerLove[kNumParams] = {
        0.51f, 0.50f, 0.82f, 0.55f,   // OSC: lekki detune, cieplejszy
        0.65f, 0.35f, 0.06f,           // FLT: jaśniejszy (melodia)
        0.02f, 0.25f, 0.52f, 0.22f,   // ENV: delikatny atak
        0.18f, 0.00f,
        0.35f, 0.25f, 0.50f, 0.90f,
    };
    presets_.push_back(makeMoroderPreset(
        "Computer Love (Kraftwerk 81)",
        kComputerLove, 128.0f, 8, 16,
        {
            {65,true},{69,true},{72,true},{77,true},   // F4 A4 C5 F5
            {76,true},{74,true},{72,true},{69,true},   // E5 D5 C5 A4
            {65,true},{67,true},{69,true},{72,true},   // F4 G4 A4 C5
            {70,true},{69,true},{67,true},{65,true},   // Bb4 A4 G4 F4
        }
    ));

    // ── Presety Jean-Michel Jarre ─────────────────────────────────────────────
    //
    // Brzmienie: ARP 2600 / VCS3 — bardzo rezonansowy filtr, "wilgotne" arpedżia.

    // ── "Oxygene Part IV" (1976) ──────────────────────────────────────────────
    // Cykliczne arpegio d-mol, 116 BPM, 16th-note grid
    static const float kOxygene[kNumParams] = {
        0.51f, 0.50f, 0.85f, 0.50f,   // OSC: piła
        0.22f, 0.82f, 0.32f,           // FLT: mocno zamknięty, wysoki Q, LFO sweep!
        0.00f, 0.22f, 0.18f, 0.18f,   // ENV
        0.12f, 0.00f,                  // LFO wolny sinus
        0.25f, 0.35f, 0.55f, 0.90f,   // FX: umiarkowany reverb
    };
    presets_.push_back(makeMoroderPreset(
        "Oxygene IV (JMJ 76)",
        kOxygene, 116.0f, 16, 16,
        {
            {50,true},{53,true},{57,true},{62,true},   // D3 F3 A3 D4
            {48,true},{52,true},{55,true},{60,true},   // C3 E3 G3 C4
            {46,true},{50,true},{53,true},{58,true},   // Bb2 D3 F3 Bb3
            {45,true},{48,true},{52,true},{57,true},   // A2 C3 E3 A3
        }
    ));

    // ── "Equinoxe Part 5" (1978) ──────────────────────────────────────────────
    // Pulsujący bas e-mol z wznoszącymi chromatics, 126 BPM
    static const float kEquinoxe[kNumParams] = {
        0.51f, 0.50f, 0.88f, 0.45f,
        0.28f, 0.68f, 0.28f,           // FLT: ciemny, wysoki Q
        0.00f, 0.20f, 0.20f, 0.16f,
        0.14f, 0.00f,
        0.22f, 0.28f, 0.48f, 0.90f,
    };
    presets_.push_back(makeMoroderPreset(
        "Equinoxe 5 (JMJ 78)",
        kEquinoxe, 126.0f, 8, 16,
        {
            {40,true},{40,true},{47,true},{40,true},   // E2 E2 B2 E2
            {43,true},{40,true},{47,true},{50,true},   // G2 E2 B2 D3
            {40,true},{40,true},{47,true},{40,true},   // E2 E2 B2 E2
            {45,true},{43,true},{42,true},{40,true},   // A2 G2 F#2 E2
        }
    ));

    // ── Presety Vangelis ──────────────────────────────────────────────────────

    // ── "Blade Runner Blues" (1982) ───────────────────────────────────────────
    // Rzadkie, przestrzenne zejście c-mol, 119 BPM — pauzy = inactive steps
    static const float kBladeRunner[kNumParams] = {
        0.555f, 0.50f, 0.80f, 0.60f,  // OSC: lekki detune, pad-like
        0.35f,  0.20f, 0.05f,          // FLT: ciemny, niski Q (pad, nie bass)
        0.20f,  0.30f, 0.65f, 0.55f,  // ENV: wolny atak, długi sustain/release
        0.10f,  0.00f,
        0.40f,  0.65f, 0.90f, 0.88f,  // FX: ogromny reverb
    };
    presets_.push_back(makeMoroderPreset(
        "Blade Runner (Vangelis 82)",
        kBladeRunner, 119.0f, 8, 16,
        {
            {48,true },{48,false},{51,true },{51,false},  // C3 — Eb3 —
            {55,true },{55,false},{53,false},{53,true },  // G3 — — F3
            {51,true },{51,false},{48,true },{48,false},  // Eb3 — C3 —
            {46,true },{46,false},{44,true },{44,false},  // Bb2 — Ab2 —
        }
    ));

    // ── "Chariots of Fire" (1981) ─────────────────────────────────────────────
    // Ikoniczny motyw D-dur, 120 BPM — trójkąt/piano tone
    static const float kChariots[kNumParams] = {
        0.50f, 0.25f, 0.82f, 0.45f,   // OSC: trojkat (piano-like)
        0.72f, 0.15f, 0.04f,           // FLT: otwarty, mało rezonansu
        0.00f, 0.35f, 0.22f, 0.32f,   // ENV: perkusyjny decay
        0.15f, 0.00f,
        0.28f, 0.45f, 0.65f, 0.90f,   // FX: wyraźny reverb (sala)
    };
    presets_.push_back(makeMoroderPreset(
        "Chariots of Fire (Vangelis 81)",
        kChariots, 120.0f, 8, 16,
        {
            {62,true },{62,true },{64,true },{62,false},  // D4 D4 E4 —
            {62,true },{62,false},{59,true },{59,false},  // D4 — B3 —
            {62,true },{62,true },{64,true },{66,true },  // D4 D4 E4 F#4
            {69,true },{69,false},{67,true },{67,false},  // A4 — G4 —
        }
    ));

    // ── Preset Harold Faltermeyer ─────────────────────────────────────────────

    // ── "Axel F" — Beverly Hills Cop (1984) ───────────────────────────────────
    // Ikoniczny hook f-mol, 120 BPM — nasalny square lead + pauzy
    static const float kAxelF[kNumParams] = {
        0.50f, 0.75f, 0.88f, 0.35f,   // OSC: square/pulse (nasalny)
        0.62f, 0.55f, 0.06f,           // FLT: średnio otwarty, wyraźny rezonans
        0.00f, 0.16f, 0.55f, 0.15f,   // ENV: szybki atak
        0.22f, 0.00f,
        0.30f, 0.18f, 0.42f, 0.90f,
    };
    presets_.push_back(makeMoroderPreset(
        "Axel F (Faltermeyer 84)",
        kAxelF, 120.0f, 8, 16,
        {
            {53,true },{53,false},{56,true },{53,true },  // F3 — Ab3 F3
            {53,true },{58,true },{58,false},{53,true },  // F3 Bb3 — F3
            {51,true },{53,true },{53,false},{53,false},  // Eb3 F3 — —
            {48,true },{48,false},{49,true },{48,true },  // C3 — Db3 C3
        }
    ));

    // ── Preset Gary Numan ─────────────────────────────────────────────────────

    // ── "Cars" (1979) ──────────────────────────────────────────────────────────
    // Zimny, mechaniczny bas eb-mol, 126 BPM — Minimoog przez Oberheim
    static const float kCars[kNumParams] = {
        0.50f, 0.50f, 0.92f, 0.20f,   // OSC: czysta piła, prawie bez OscB
        0.42f, 0.52f, 0.04f,           // FLT: zamknięty, wyraźny Q
        0.00f, 0.16f, 0.25f, 0.10f,   // ENV: błyskawiczny, minimalny sustain
        0.10f, 0.00f,
        0.10f, 0.08f, 0.30f, 0.90f,   // FX: prawie suche (zimny efekt)
    };
    presets_.push_back(makeMoroderPreset(
        "Cars (Gary Numan 79)",
        kCars, 126.0f, 8, 16,
        {
            {51,true },{51,true },{51,false},{51,true },  // Eb3 Eb3 — Eb3
            {51,true },{46,true },{46,false},{54,true },  // Eb3 Bb2 — Gb3
            {51,true },{51,true },{51,false},{51,true },  // Eb3 Eb3 — Eb3
            {49,true },{46,true },{44,true },{42,true },  // Db3 Bb2 Ab2 Gb2
        }
    ));

    // ── Presety Depeche Mode (wczesne) ────────────────────────────────────────
    //
    // Brzmienie: Synclavier / PPG Wave 2.2 — chłodny cyfrowy ton,
    // filtr mniej rezonansowy niż Moog, rytmicznie agresywne.

    // ── "Everything Counts" (1983) ────────────────────────────────────────────
    // Bas g-mol z akcentem na synkopy, 108 BPM
    static const float kEverythingCounts[kNumParams] = {
        0.50f, 0.50f, 0.88f, 0.40f,
        0.55f, 0.40f, 0.06f,           // FLT: jasny (PPG cyfrowy)
        0.00f, 0.20f, 0.42f, 0.18f,
        0.18f, 0.00f,
        0.20f, 0.18f, 0.42f, 0.90f,
    };
    presets_.push_back(makeMoroderPreset(
        "Everything Counts (DM 83)",
        kEverythingCounts, 108.0f, 8, 16,
        {
            {43,true },{43,true },{46,true },{43,true },  // G2 G2 Bb2 G2
            {43,true },{41,true },{43,true },{43,false},  // G2 F2 G2 —
            {43,true },{43,true },{46,true },{43,true },  // G2 G2 Bb2 G2
            {39,true },{41,true },{43,true },{44,true },  // Eb2 F2 G2 Ab2
        }
    ));

    // ── "Master and Servant" (1984) ───────────────────────────────────────────
    // Agresywny bas G#-mol z pauzami, 126 BPM
    static const float kMasterServant[kNumParams] = {
        0.50f, 0.50f, 0.90f, 0.35f,
        0.30f, 0.65f, 0.08f,           // FLT: ciemny, wysoki Q
        0.00f, 0.16f, 0.28f, 0.12f,   // ENV: szybki, twardy
        0.20f, 0.00f,
        0.18f, 0.15f, 0.40f, 0.90f,
    };
    presets_.push_back(makeMoroderPreset(
        "Master & Servant (DM 84)",
        kMasterServant, 126.0f, 8, 16,
        {
            {44,true },{44,true },{44,false},{44,true },  // Ab2 Ab2 — Ab2
            {44,true },{42,true },{44,true },{44,true },  // Ab2 F#2 Ab2 Ab2
            {44,false},{44,true },{44,true },{46,true },  // — Ab2 Ab2 Bb2
            {44,true },{42,true },{40,true },{39,true },  // Ab2 F#2 E2 Eb2
        }
    ));

    // ── Preset John Carpenter ─────────────────────────────────────────────────

    // ── "Halloween Theme" (1978) ──────────────────────────────────────────────
    // Złowieszczy motyw 5/4 → zmapowany na 16 kroków z pauzami, 139 BPM
    // Brzmienie: sinus/trójkąt przez pogłos (piano elektryczne Carpenter'a)
    static const float kHalloween[kNumParams] = {
        0.50f, 0.25f, 0.85f, 0.35f,   // OSC: trojkat — niewinny ale złowrogi
        0.40f, 0.30f, 0.04f,           // FLT: półotwarty
        0.05f, 0.40f, 0.40f, 0.42f,   // ENV: wolny atak + długi release (duch)
        0.12f, 0.00f,
        0.20f, 0.62f, 0.85f, 0.88f,   // FX: ogromny reverb dla atmosfery horroru
    };
    presets_.push_back(makeMoroderPreset(
        "Halloween (Carpenter 78)",
        kHalloween, 139.0f, 8, 16,
        {
            {76,true },{72,true },{72,false},{72,false},  // E5 C5 — —
            {76,true },{72,true },{72,false},{76,true },  // E5 C5 — E5
            {76,false},{76,false},{76,false},{76,false},  // — — — —
            {76,true },{72,true },{72,false},{72,false},  // E5 C5 — —
        }
    ));
}

// ── PresetManager ─────────────────────────────────────────────────────────────

PresetManager::PresetManager() {
    buildFactoryPresets();
    loadFromFile();
}

juce::StringArray PresetManager::getNames() const {
    juce::StringArray names;
    for (auto& p : presets_)
        names.add(p.name);
    return names;
}

const PresetManager::Preset* PresetManager::getPreset(int index) const {
    if (index < 0 || index >= (int)presets_.size()) return nullptr;
    return &presets_[index];
}

const PresetManager::Preset* PresetManager::getPreset(const juce::String& name) const {
    for (auto& p : presets_)
        if (p.name == name) return &p;
    return nullptr;
}

void PresetManager::savePreset(const juce::String& name,
                               const std::vector<ParamValue>& params) {
    // Szukaj istniejacego uzytkownika o tej nazwie
    for (auto& p : presets_) {
        if (p.name == name && !p.isFactory) {
            p.params = params;
            saveToFile();
            return;
        }
    }
    Preset p;
    p.name      = name;
    p.isFactory = false;
    p.params    = params;
    presets_.push_back(std::move(p));
    saveToFile();
}

bool PresetManager::deletePreset(const juce::String& name) {
    for (auto it = presets_.begin(); it != presets_.end(); ++it) {
        if (it->name == name && !it->isFactory) {
            presets_.erase(it);
            saveToFile();
            return true;
        }
    }
    return false;
}

// ── Plik uzytkownika ──────────────────────────────────────────────────────────

juce::File PresetManager::userFile() const {
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
               .getChildFile("SynthAppG2")
               .getChildFile("presets.json");
}

void PresetManager::saveToFile() {
    auto f = userFile();
    f.getParentDirectory().createDirectory();

    nlohmann::json j;
    j["version"] = "synthapp-0.1";
    auto& arr = j["presets"] = nlohmann::json::array();

    for (auto& p : presets_) {
        if (p.isFactory) continue;  // fabryczne nie zapisujemy
        nlohmann::json pj;
        pj["name"] = p.name.toStdString();
        auto& pars = pj["params"] = nlohmann::json::array();
        for (auto& pv : p.params)
            pars.push_back({ {"mod", pv.moduleId}, {"param", pv.paramIdx}, {"val", pv.value} });

        if (p.hasSeq) {
            pj["seq"]["bpm"]   = p.seqBPM;
            pj["seq"]["div"]   = p.seqDiv;
            pj["seq"]["steps"] = p.seqSteps;
            auto& sp = pj["seq"]["pattern"] = nlohmann::json::array();
            for (auto& s : p.seqPattern)
                sp.push_back({ {"n", s.note}, {"a", s.active} });
        }
        arr.push_back(pj);
    }

    f.replaceWithText(j.dump(2));
}

void PresetManager::loadFromFile() {
    auto f = userFile();
    if (!f.existsAsFile()) return;

    try {
        nlohmann::json j = nlohmann::json::parse(f.loadFileAsString().toStdString());
        if (!j.contains("presets")) return;

        for (auto& pj : j["presets"]) {
            Preset p;
            p.name      = juce::String(pj["name"].get<std::string>());
            p.isFactory = false;
            for (auto& pv : pj["params"])
                p.params.push_back({ pv["mod"].get<int>(),
                                     pv["param"].get<int>(),
                                     pv["val"].get<float>() });
            if (pj.contains("seq")) {
                auto& sq  = pj["seq"];
                p.hasSeq  = true;
                p.seqBPM  = sq.value("bpm",   120.0f);
                p.seqDiv  = sq.value("div",   16);
                p.seqSteps= sq.value("steps", 8);
                if (sq.contains("pattern"))
                    for (auto& s : sq["pattern"])
                        p.seqPattern.push_back({ s.value("n",60), s.value("a",true) });
            }
            presets_.push_back(std::move(p));
        }
    } catch (...) {}
}
