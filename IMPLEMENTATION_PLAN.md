# SynthApp — Plan Implementacji
## Wierne Odwzorowanie Nord Modular G2 z Prostym Interfejsem Fizycznym

---

## Filozofia Projektu

> "Interfejs powinien znikać — użytkownik ma czuć się przy syntezatorze, nie przy programie."

Zamiast pełnego edytora modularnego (złożoność G2 Editor), tworzymy:
- **Panel fizyczny** — wygląda i działa jak hardware G2
- **Jeden ekran patcha** — tylko aktywny patch, bez drzewka plików
- **Knobs jak śruby** — obrót myszą/touchem, double-click = reset
- **Kable jak druty** — przeciągnij od portu do portu, kolory jak w G2

---

## Stos Technologiczny

| Warstwa | Wybór | Dlaczego |
|---|---|---|
| Język | **C++20** | DSP bez GC, SIMD, deterministyczny RT |
| Framework UI + Audio | **JUCE 8** | VST3/AU/Standalone w jednym, własny renderer |
| DSP matematyka | **Eigen 3** (SIMD) | Wektoryzacja filtrów i oscylatorów |
| Format patcha | **JSON (nlohmann)** | Czytelny, + bridge do .pch2 |
| Testy DSP | **Catch2 + KissFFT** | Analiza widmowa bez FFTW dependency |
| Build | **CMake 3.25 + CPM** | Jedno polecenie, wszystkie dep auto |
| CI | **GitHub Actions** | Build Linux/macOS/Windows na każdy PR |

---

## Układ Interfejsu — "Physical Panel"

```
╔══════════════════════════════════════════════════════════════════════════╗
║  N O R D   M O D U L A R   G 2                              [CLAVIA]    ║
╠══════════════════════════════════════════════════════════════════════════╣
║                                                                          ║
║  ┌─ SLOT A ──────────────────────────────────────────────────────────┐  ║
║  │                                                                    │  ║
║  │  ┌─[OscA]──────┐   ┌─[FltLP]──────┐   ┌─[Env ADSR]───┐          │  ║
║  │  │ ◎ Pitch     │   │ ◎ Cutoff     │   │ ◎ Attack     │          │  ║
║  │  │ ◎ Fine      │──►│ ◎ Resonance  │   │ ◎ Decay      │          │  ║
║  │  │ ◎ Wave [▼]  │   │ ◎ KBD Track  │◄──│ ◎ Sustain    │          │  ║
║  │  │ ○ audio out │   │ ○ audio out  │   │ ◎ Release    │          │  ║
║  │  └─────────────┘   └──────────────┘   │ ○ env out    │          │  ║
║  │                                        └──────────────┘          │  ║
║  │                    ════════════════════════════════               │  ║
║  │                    ◄ kabel audio (żółty) ════════════►            │  ║
║  └────────────────────────────────────────────────────────────────────┘  ║
║                                                                          ║
║  ┌─ PERFORMANCE ─────────────────────────────────────────────────────┐  ║
║  │  [A] [B] [C] [D]    MASTER VOL: ◎    PITCH: ══╗  MOD: ══╗        │  ║
║  │   ●                             │              ║         ║        │  ║
║  └────────────────────────────────────────────────────────────────────┘  ║
╚══════════════════════════════════════════════════════════════════════════╝
```

### Zasady UI

1. **Ciemnoszare tło** (#1a1a1a) — identyczny kontrast jak hardware G2
2. **Moduły jako karty** — zaokrąglone rogi, lekki cień, kolor kategorii:
   - Oscylatory → bordowy (#8B1A1A)
   - Filtry → ciemnoniebieski (#1A3A6B)
   - Koperty/LFO → ciemnozielony (#1A5C2A)
   - Efekty → ciemnofioletowy (#3A1A6B)
   - Mikser/Out → grafitowy (#3A3A3A)
3. **Kable** → żółte (audio), niebieskie (CV), pomarańczowe (gate)
4. **Knobs** → 270° zakres, obrót w górę = większa wartość, prawy klik = menu

---

## Iteracje Implementacji

### Iteracja 1 — "Pierwszy Dźwięk" (Tydzień 1–3)

**Cel:** Aplikacja standalone uruchamia się i gra jeden patch z OscA → FltLP → Out.

#### 1.1 Setup projektu
```
synthapp/
├── CMakeLists.txt
├── src/
│   ├── Main.cpp
│   ├── PluginProcessor.h/.cpp
│   └── PluginEditor.h/.cpp
└── CMakeLists.txt
```

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.25)
project(SynthApp VERSION 0.1.0)

include(cmake/CPM.cmake)
CPMAddPackage("gh:juce-framework/JUCE@8.0.0")
CPMAddPackage("gh:nlohmann/json@3.11.3")
CPMAddPackage("gh:catchorg/Catch2@3.5.0")

juce_add_plugin(SynthApp
    PLUGIN_MANUFACTURER_CODE Snth
    PLUGIN_CODE Snth
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "SynthApp G2"
)
```

#### 1.2 Silnik audio — podstawowy szkielet

```cpp
// src/engine/AudioEngine.h
class AudioEngine : public juce::AudioProcessor {
public:
    void prepareToPlay(double sampleRate, int blockSize) override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

private:
    double sampleRate_ = 96000.0;  // G2 native rate
    int    blockSize_  = 64;       // małe bloki = niska latencja
    VoicePool voices_;
    ModuleGraph graph_;
};
```

#### 1.3 Moduł bazowy

```cpp
// src/modules/Module.h
class Module {
public:
    virtual ~Module() = default;

    // Wywoływane raz przy zmianie parametrów
    virtual void prepare(double sampleRate, int blockSize) = 0;

    // Wywoływane w audio thread — ŻADNYCH alokacji
    virtual void process(float* audioIn,  float* audioOut,
                         float* cvIn,     float* cvOut,
                         int numSamples) = 0;

    // Parametry jako znormalizowane wartości [0..1]
    virtual void setParam(int index, float value) = 0;
    virtual float getParam(int index) const = 0;

    struct PortInfo { std::string name; enum Type { Audio, CV, Gate } type; bool isOutput; };
    virtual std::vector<PortInfo> getPorts() const = 0;
};
```

#### 1.4 Pierwszy oscylator — OscA

```cpp
// src/modules/oscillators/OscA.cpp
// Algorytm: PolyBLEP sawtooth/square + pure sine/triangle

class OscA : public Module {
    enum Param { kPitch, kFine, kWaveform, kPWM, kNumParams };

    // PolyBLEP — eliminuje aliasing bez oversamplingu
    float polyblep(float t, float dt) {
        if (t < dt) {
            t /= dt;
            return t + t - t * t - 1.0f;
        } else if (t > 1.0f - dt) {
            t = (t - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        }
        return 0.0f;
    }

    void process(float*, float* audioOut, float* cvIn, float*, int n) override {
        float freq = midiToHz(params_[kPitch] * 127.0f + params_[kFine] - 0.5f);
        if (cvIn) freq *= std::exp2(cvIn[0]); // 1V/oct CV
        float dt = freq / sampleRate_;

        for (int i = 0; i < n; ++i) {
            // Sawtooth z PolyBLEP
            float saw = 2.0f * phase_ - 1.0f;
            saw -= polyblep(phase_, dt);
            audioOut[i] = saw;

            phase_ += dt;
            if (phase_ >= 1.0f) phase_ -= 1.0f;
        }
    }

    float phase_ = 0.0f;
    float params_[kNumParams] = { 0.5f, 0.5f, 0.0f, 0.5f };
};
```

---

### Iteracja 2 — "Kompletny Głos" (Tydzień 4–6)

**Cel:** OscA + OscB (sync) + FltNord + FltLP + ADSR + LfoA = pełny głos syntezatora.

#### 2.1 Zero-Delay Feedback Ladder Filter (FltLP)

```cpp
// Implementacja wg Zavalishin "The Art of VA Filter Design" §6.5
class FltLP : public Module {
    // 4 stany one-pole sekcji
    float s[4] = {};

    // Nieliniowość Tanh (LUT dla wydajności)
    float tanhLUT(float x) { /* 4096-point LUT */ }

    void process(float* audioIn, float* audioOut, float* cvIn, float*, int n) {
        float fc = params_[kCutoff];  // [0..1] → freq
        if (cvIn) fc += cvIn[0] * params_[kKbdTrack];
        fc = std::clamp(fc, 0.0f, 1.0f);

        float cutHz = 20.0f * std::pow(1000.0f, fc);  // 20Hz..20kHz
        float g = std::tan(M_PI * cutHz / sampleRate_);
        float k = 4.0f * params_[kResonance];          // 0..4 (samoosc. przy 4)

        // ZDF rozwiązanie (exact, brak opóźnienia)
        float g1 = g / (1.0f + g);
        for (int i = 0; i < n; ++i) {
            float x = audioIn[i];
            // ... (pełna implementacja ZDF 4-pole)
            audioOut[i] = s[3];
        }
    }
};
```

#### 2.2 ADSR z krzywymi wykładniczymi

```cpp
class EnvADSR : public Module {
    enum Stage { Off, Attack, Decay, Sustain, Release };
    Stage stage_ = Off;
    float value_ = 0.0f;

    // G2 używa krzywych wykładniczych — bardziej muzyczne niż liniowe
    float expCurve(float t, float rate) {
        return 1.0f - std::exp(-t * rate);
    }

    void noteOn()  { stage_ = Attack; }
    void noteOff() { stage_ = Release; }
};
```

#### 2.3 Graf modułów — topological sort

```cpp
// src/engine/ModuleGraph.cpp
class ModuleGraph {
public:
    void addModule(int id, std::unique_ptr<Module> m);
    void connect(int fromModule, int fromPort, int toModule, int toPort);
    void disconnect(int fromModule, int fromPort, int toModule, int toPort);

    // Wywołaj po każdej zmianie patcha (w message thread)
    void recompileOrder();

    // Wywołaj w audio thread — ŻADNYCH alokacji, ŻADNYCH locków
    void process(int numSamples, const NoteEvent* events, int numEvents);

private:
    // Kahn's algorithm — O(V+E)
    std::vector<int> topoOrder_;

    // Double buffer — swap atomowo
    std::atomic<int> activeBuffer_{ 0 };
    ProcessingState buffers_[2];
};
```

---

### Iteracja 3 — "UI Panel" (Tydzień 7–10)

**Cel:** Graficzny interfejs odwzorowujący fizyczny panel G2.

#### 3.1 Komponenty UI

```
src/ui/
├── PanelView.h/.cpp        # Główny widok — ciemne tło G2
├── ModuleView.h/.cpp       # Karta modułu z knobami
├── KnobView.h/.cpp         # Fizyczny pokrętło (270°, animowane)
├── CableView.h/.cpp        # Renderowanie kabli jako krzywe Beziera
├── PortView.h/.cpp         # Gniazdo kabla (kliknij i przeciągnij)
├── SlotTabView.h/.cpp      # Zakładki A/B/C/D
└── PerformanceBar.h/.cpp   # CPU, głosy, master vol, pitch/mod wheel
```

#### 3.2 KnobView — fizyczny feel

```cpp
class KnobView : public juce::Component {
    // Obrót myszy: drag góra/dół zmienia wartość
    // Shift + drag = fine control (100x wolniej)
    // Double click = reset do domyślnej wartości
    // Prawy klik = { "Enter Value...", "Assign MIDI CC", "Reset" }
    // Tooltip przy hover = nazwa parametru + wartość z jednostką

    void paint(juce::Graphics& g) override {
        // Rysuj okrąg knoba
        // Rysuj wskaźnik (linia od środka pod kątem 270° * value)
        // Rysuj subtelny łuk tła (arc)
        // Kolor: domyślnie srebrny, aktywny = podświetlenie
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        float delta = -e.getDistanceFromDragStartY() * 0.005f;
        if (e.mods.isShiftDown()) delta *= 0.01f;
        setValue(std::clamp(value_ + delta, 0.0f, 1.0f));
    }
};
```

#### 3.3 CableView — kable jak w G2

```cpp
class CableView : public juce::Component {
    struct Cable {
        juce::Point<float> from, to;
        juce::Colour color;  // yellow=audio, blue=CV, orange=gate
        float slack = 0.3f;  // fizyczne zwisanie kabla
    };

    void paint(juce::Graphics& g) override {
        for (auto& cable : cables_) {
            // Krzywa Beziera z naturalnym zwisem (symulacja grawitacji)
            float midY = std::max(cable.from.y, cable.to.y) + cable.slack * 80.0f;
            juce::Point<float> ctrl1 { cable.from.x, midY };
            juce::Point<float> ctrl2 { cable.to.x, midY };

            juce::Path path;
            path.startNewSubPath(cable.from);
            path.cubicTo(ctrl1, ctrl2, cable.to);

            g.setColour(cable.color.withAlpha(0.85f));
            g.strokePath(path, juce::PathStrokeType(2.5f,
                juce::PathStrokeType::curved,
                juce::PathStrokeType::rounded));
        }
    }
};
```

#### 3.4 Układ modułów na panelu

**Zasada:** Moduły ułożone w rzędach (nie swobodnie jak w DAW) — jak na prawdziwym panelu hardware.

```
Rząd 1: [Oscylatory] [Oscylatory] [Oscylatory]
Rząd 2: [Filtry]     [Filtry]     [Envelope]  [LFO]
Rząd 3: [Efekty]     [Efekty]     [Mix/Out]
```

Użytkownik może:
- Dodać moduł z biblioteki (prawy klik na panel → menu)
- Przesunąć moduł (drag nagłówka)
- Usunąć moduł (prawy klik → Delete)
- Połączyć porty kablem (klik na port → kursor zmienia się → klik na cel)

---

### Iteracja 4 — "Pełne Brzmienie G2" (Tydzień 11–14)

**Cel:** Wszystkie kluczowe moduły G2 działają, import patchy .pch2.

#### 4.1 Kompletna lista modułów do implementacji

**OSCYLATORY**
```
OscA   — Sine/Triangle/Sawtooth/Square/Pulse + PWM        [PRIORYTET 1]
OscB   — jak OscA + hard sync od slave                    [PRIORYTET 1]
OscC   — Wavetable 256-point z mipmapping                 [PRIORYTET 2]
OscD   — FM (ratio, index, feedback)                      [PRIORYTET 2]
Noise  — White/Pink/Red szum                              [PRIORYTET 1]
ExtIn  — Wejście audio z hosta                            [PRIORYTET 3]
```

**FILTRY**
```
FltLP    — ZDF Ladder 4-pole (ciepło Mooga)              [PRIORYTET 1]
FltNord  — ZDF State-Variable 2/4-pole LP/HP/BP/Notch    [PRIORYTET 1]
FltHP    — High-pass, butterworth                         [PRIORYTET 1]
FltBP    — Band-pass                                      [PRIORYTET 2]
FltVoice — Formant (5 samogłosek, morph)                 [PRIORYTET 2]
FltPhase — Allpass phaser 2/4/6/8 sekcji                 [PRIORYTET 2]
FltComb  — Flanger/comb (krótkie opóźnienie + feedback)  [PRIORYTET 3]
```

**KOPERTY i LFO**
```
EnvADSR     — Attack/Decay/Sustain/Release, exp curves   [PRIORYTET 1]
EnvAD       — One-shot attack/decay                      [PRIORYTET 1]
EnvFollow   — RMS envelope follower                      [PRIORYTET 2]
LfoA        — Sine/Tri/Saw/Square/S&H, tempo sync        [PRIORYTET 1]
LfoB        — Jak LfoA + phase startowy                  [PRIORYTET 2]
LfoC        — Wavetable LFO                              [PRIORYTET 3]
```

**EFEKTY**
```
FxReverb    — Schroeder roomsim, size/damp/mix           [PRIORYTET 2]
FxDelay     — Stereo delay, tempo sync, feedback         [PRIORYTET 2]
FxChorus    — Multi-tap modulated delay                  [PRIORYTET 2]
FxDistort   — Waveshaper: softclip/hardclip/foldback     [PRIORYTET 2]
FxCompressor— Peak/RMS, ratio/thresh/attack/release      [PRIORYTET 2]
FxEQ        — 3-band EQ (low shelf, mid peak, high shelf)[PRIORYTET 3]
```

**MIKSER i ROUTING**
```
Mix2-1   — 2 wejścia → 1 wyjście, level A+B             [PRIORYTET 1]
Mix4-1   — 4 wejścia → 1 wyjście                        [PRIORYTET 1]
Mix8-1   — 8 wejść → 1 wyjście                          [PRIORYTET 2]
Crossfade — A↔B morph po CV                             [PRIORYTET 2]
Pan      — L/R pan po CV                                 [PRIORYTET 1]
VCA      — Wzmacniacz sterowany CV (bramka głosu)        [PRIORYTET 1]
```

**SEKWENCER i LOGIKA**
```
SeqNote  — 16-step melodyczny, glide, rest              [PRIORYTET 2]
SeqCtrl  — 16-step CV, bipolarny                        [PRIORYTET 2]
LogicAnd — Gate AND                                     [PRIORYTET 3]
LogicOr  — Gate OR                                      [PRIORYTET 3]
ClkDiv   — Clock divider (/2 /4 /8 /16 /32)            [PRIORYTET 2]
ClkGen   — Wewnętrzny zegar BPM                        [PRIORYTET 1]
```

**SPECJALNE**
```
KeyNote  — MIDI → Note CV + Gate                        [PRIORYTET 1]
KeyVelo  — MIDI → Velocity CV                           [PRIORYTET 1]
KeyAfter — MIDI → Aftertouch CV                         [PRIORYTET 2]
PitchBnd — MIDI → Pitch Bend CV                         [PRIORYTET 1]
ModWheel — MIDI → Mod Wheel CV                          [PRIORYTET 1]
Out      — Stereo wyjście z poziomu                     [PRIORYTET 1]
```

#### 4.2 Import patchy .pch2

```python
# tools/pch2_to_json.py — bridge używający g2ools
# Uruchamiany jako subprocess z C++ przy Load

import g2ools
import json, sys

def convert(pch2_path):
    patch = g2ools.Pch2File(pch2_path)
    result = {
        "version": "g2-import",
        "slots": []
    }
    for slot in patch.slots:
        modules = []
        for m in slot.modules:
            modules.append({
                "id": m.index,
                "type": m.type_name,
                "params": {p.name: p.value for p in m.params},
                "position": {"row": m.row, "col": m.col}
            })
        cables = []
        for c in slot.cables:
            cables.append({
                "from": {"module": c.module_from, "port": c.output},
                "to":   {"module": c.module_to,   "port": c.input},
                "type": ["audio","cv","gate"][c.color]
            })
        result["slots"].append({"modules": modules, "cables": cables})

    print(json.dumps(result))

convert(sys.argv[1])
```

---

### Iteracja 5 — "Plugin + Stabilność" (Tydzień 15–16)

**Cel:** VST3/AU działa w Ableton/Reaper, brak crashy, CPU < 20%.

#### 5.1 Checklist przed wydaniem

```
Performance:
  [ ] 32 głosy @ złożony patch < 20% CPU (Apple M1)
  [ ] Latencja audio: ≤ 5ms przy block size 64
  [ ] Brak alokacji w audio thread (Xcode Allocations check)
  [ ] Brak wyścigów (ThreadSanitizer clean)

Brzmienie:
  [ ] FltLP: błąd odpowiedzi < 0.5 dB vs. reference G2 recording
  [ ] OscA sawtooth: SINAD > 90 dB (brak aliasingu)
  [ ] ADSR timing: ≤ 1ms odchyłka od zadanego czasu
  [ ] ABX test: > 75% zgodności z brzmieniem G2

Stabilność:
  [ ] 24h stress test (losowe nuty, parametry)
  [ ] Import 20 różnych patchy .pch2 bez błędu
  [ ] Plug-in scan w Ableton/Reaper/Logic
  [ ] Brak memory leaks (Valgrind/Instruments clean)
```

---

## Struktura Katalogów (Finalna)

```
synthapp/
├── CMakeLists.txt
├── cmake/
│   └── CPM.cmake
│
├── src/
│   ├── Main.cpp                      # Standalone entry
│   ├── PluginProcessor.h/.cpp        # AudioProcessor (JUCE)
│   ├── PluginEditor.h/.cpp           # UI root
│   │
│   ├── engine/
│   │   ├── AudioEngine.h/.cpp        # Główna pętla audio
│   │   ├── ModuleGraph.h/.cpp        # DAG + topological sort
│   │   ├── VoicePool.h/.cpp          # 32 głosy, steal policy
│   │   ├── SlotManager.h/.cpp        # Sloty A/B/C/D
│   │   └── NoteEvent.h               # MIDI → CV/Gate events
│   │
│   ├── modules/
│   │   ├── Module.h                  # Interfejs bazowy
│   │   ├── ModuleRegistry.h/.cpp     # Fabryka (name → Module*)
│   │   ├── oscillators/
│   │   │   ├── OscA.h/.cpp
│   │   │   ├── OscB.h/.cpp
│   │   │   ├── OscC.h/.cpp           # Wavetable
│   │   │   ├── OscD.h/.cpp           # FM
│   │   │   └── Noise.h/.cpp
│   │   ├── filters/
│   │   │   ├── FltLP.h/.cpp          # ZDF Ladder
│   │   │   ├── FltNord.h/.cpp        # ZDF State-Variable
│   │   │   ├── FltVoice.h/.cpp
│   │   │   └── FltPhase.h/.cpp
│   │   ├── envelopes/
│   │   │   ├── EnvADSR.h/.cpp
│   │   │   ├── EnvAD.h/.cpp
│   │   │   └── EnvFollow.h/.cpp
│   │   ├── lfos/
│   │   │   ├── LfoA.h/.cpp
│   │   │   └── LfoB.h/.cpp
│   │   ├── effects/
│   │   │   ├── FxReverb.h/.cpp
│   │   │   ├── FxDelay.h/.cpp
│   │   │   ├── FxChorus.h/.cpp
│   │   │   └── FxDistort.h/.cpp
│   │   ├── mixers/
│   │   │   ├── Mix4to1.h/.cpp
│   │   │   ├── VCA.h/.cpp
│   │   │   └── Pan.h/.cpp
│   │   ├── sequencers/
│   │   │   ├── SeqNote.h/.cpp
│   │   │   └── SeqCtrl.h/.cpp
│   │   └── midi/
│   │       ├── KeyNote.h/.cpp        # MIDI → CV+Gate
│   │       ├── KeyVelo.h/.cpp
│   │       └── ModWheel.h/.cpp
│   │
│   ├── patch/
│   │   ├── PatchFormat.h/.cpp        # JSON serialization
│   │   └── CableSystem.h/.cpp        # Port connections
│   │
│   └── ui/
│       ├── PanelView.h/.cpp          # Główny panel
│       ├── ModuleView.h/.cpp         # Karta modułu
│       ├── KnobView.h/.cpp           # Pokrętło fizyczne
│       ├── CableView.h/.cpp          # Kable Beziera
│       ├── PortView.h/.cpp           # Gniazdo kabla
│       ├── SlotTabView.h/.cpp        # A/B/C/D tabs
│       └── ModuleBrowserView.h/.cpp  # Lista modułów do dodania
│
├── tests/
│   ├── test_osc_aliasing.cpp         # FFT → SINAD > 90dB
│   ├── test_filter_response.cpp      # Frequency sweep
│   ├── test_envelope_timing.cpp      # ms accuracy
│   ├── test_graph_topo.cpp           # DAG correctness
│   └── reference_audio/             # Nagrania G2 do ABX
│
└── tools/
    ├── pch2_to_json.py               # .pch2 → JSON (g2ools)
    └── abx_compare.py               # ABX audio comparison
```

---

## Harmonogram (16 Tygodni)

```
Tydzień  1-2  │ Setup CMake/JUCE, ModuleGraph szkielet, OscA daje dźwięk
Tydzień  3-4  │ FltLP (Ladder), EnvADSR, VCA, KeyNote → gra patch
Tydzień  5-6  │ FltNord, OscB (sync), LfoA, Mix4-1, Pan, Out
Tydzień  7-8  │ UI: PanelView, KnobView, CableView — wizualny panel G2
Tydzień  9-10 │ OscC (wavetable), OscD (FM), FltVoice, FltPhase
Tydzień 11-12 │ Efekty: Reverb, Delay, Chorus, Distortion, Compressor
Tydzień 13-14 │ Sekwencer, import .pch2, ModuleBrowser, 4 sloty
Tydzień 15-16 │ VST3/AU, testy ABX, optymalizacja CPU, release build
```

---

## Definicja "Gotowe"

Aplikacja jest gotowa gdy:
1. **Brzmi jak G2** — ABX test: przesłuchujący nie rozróżnia > 75% przypadków
2. **Działa w czasie rzeczywistym** — 32 głosy, CPU < 20% na Apple M1
3. **Importuje patche** — 95% patchy .pch2 z community wczytuje się poprawnie
4. **Interfejs jest intuicyjny** — nowy użytkownik tworzy patch w < 5 minut
5. **Stabilna** — 0 crashy w 24h stress teście

---

## Referencje Algorytmów

| Algorytm | Źródło |
|---|---|
| ZDF Ladder Filter | Zavalishin — *The Art of VA Filter Design* (2018) |
| PolyBLEP Oscillator | Valimaki & Pakarinen (2007) |
| Wavetable mipmapping | Andrew Simper — *Wavetable Synthesis* notes |
| Schroeder Reverb | Schroeder (1962) + Moorer extensions |
| Allpass Phaser | Regalia (1988) |
| FM Synthesis | Chowning (1973) — CCRMA paper |
| Exponential ADSR | Roli/JUCE DSP documentation |
