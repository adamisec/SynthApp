# SynthApp G2

[Polski](#polski) | [English](#english)

---

## Opis

SynthApp G2 to syntezator modularny napisany w C++20, inspirowany Nord Modular G2 firmy Clavia — legendarnym cyfrowym syntezatorem modularnym z lat 2002–2014. Celem projektu jest wierne odtworzenie charakterystycznego brzmienia G2 w postaci nowoczesnej wtyczki audio VST3/AU oraz aplikacji standalone, działającej w czasie rzeczywistym na 96 kHz i 24-bit.

Silnik DSP zbudowany jest na grafie modułów (DAG — Directed Acyclic Graph) z topologicznym sortowaniem, co gwarantuje deterministyczny i bezpieczny czas rzeczywisty bez alokacji pamięci w wątku audio. System obsługuje do 32 głosów polifonii z mechanizmem voice stealing oraz 4 niezależne sloty patchów (A/B/C/D), identycznie jak oryginalne urządzenie.

Projekt zawiera ponad 50 modułów DSP podzielonych na kategorie: oscylatory (PolyBLEP, wavetable, FM, hard sync), filtry (ZDF Ladder 4-pole w stylu Mooga, ZDF State-Variable, formantowy, faser), koperty i LFO z krzywymi wykładniczymi, efekty (reverb, delay, chorus, dystorsja, kompresor, EQ), mikser i routing sygnałów, sekwencer krokowy, arpeggiator oraz pełne mapowanie MIDI na sygnały CV i Gate.

Interfejs graficzny odwzorowuje fizyczny panel hardware G2 — ciemnoszare tło, kolorowe moduły według kategorii oraz kable rysowane jako krzywe Béziera ze zwisem imitującym prawdziwe kable patch. Obsługiwany jest import i eksport natywnego formatu patchy Clavii (.pch2) przez Python bridge oparty na bibliotece g2ools.

Aplikacja zbudowana jest w oparciu o JUCE 8 i CMake 3.25 z automatycznym pobieraniem zależności (CPM). Testy obejmują analizę widmową oscylatorów (SINAD > 90 dB), odpowiedź częstotliwościową filtrów (< 0,5 dB błędu do 20 kHz), dokładność ADSR (< 1 ms) oraz testy grafu modułów. Projekt jest rozwijany i testowany na Linux (Ubuntu/Debian). Licencja MIT.

## Description

SynthApp G2 is a modular synthesizer written in C++20, inspired by the Nord Modular G2 by Clavia — a legendary digital modular synthesizer from 2002–2014. The goal of the project is to faithfully recreate the characteristic G2 sound as a modern VST3/AU audio plugin and standalone application, running in real time at 96 kHz / 24-bit.

The DSP engine is built on a module graph (DAG — Directed Acyclic Graph) with topological sorting, ensuring deterministic and real-time-safe processing with no memory allocation in the audio thread. The system supports up to 32 voices of polyphony with voice stealing, and 4 independent patch slots (A/B/C/D), identical to the original hardware.

The project includes over 50 DSP modules across categories: oscillators (PolyBLEP, wavetable, FM, hard sync), filters (ZDF Ladder 4-pole Moog-style, ZDF State-Variable, formant, phaser), envelopes and LFOs with exponential curves, effects (reverb, delay, chorus, distortion, compressor, EQ), signal mixer and routing, step sequencer, arpeggiator, and full MIDI-to-CV/Gate mapping.

The graphical interface mirrors the physical G2 hardware panel — dark grey background, colour-coded module cards by category, and cables rendered as Bezier curves with natural sag simulating real patch cables. Native Clavia patch format (.pch2) import and export is supported via a Python bridge using the g2ools library.

The application is built with JUCE 8 and CMake 3.25 with automatic dependency fetching (CPM). Tests cover oscillator spectral analysis (SINAD > 90 dB), filter frequency response (< 0.5 dB error up to 20 kHz), ADSR timing accuracy (< 1 ms), and module graph correctness. The project is developed and tested on Linux (Ubuntu/Debian). MIT License.

---

# Polski

Syntezator modularny inspirowany Nord Modular G2 firmy Clavia (2002–2014).
Celem projektu jest wierne odtworzenie brzmienia G2 w postaci wtyczki VST3/AU
oraz aplikacji standalone, działającej w czasie rzeczywistym na 96 kHz / 24-bit.

---

## Spis treści

- [Funkcje](#funkcje)
- [Architektura](#architektura)
- [Moduly](#moduly)
- [Interfejs uzytkownika](#interfejs-uzytkownika)
- [Budowanie](#budowanie)
- [Uruchamianie](#uruchamianie)
- [Testy i walidacja brzmienia](#testy-i-walidacja-brzmienia)
- [Import patchy .pch2](#import-patchy-pch2)
- [Kompatybilnosc z G2](#kompatybilnosc-z-g2)
- [Harmonogram](#harmonogram)
- [Ryzyka i mitigacje](#ryzyka-i-mitigacje)
- [Struktura projektu](#struktura-projektu)
- [Referencje algorytmow](#referencje-algorytmow)
- [Licencja](#licencja)

---

## Funkcje

- Silnik DSP zgodny z architektura G2 (96 kHz, bloki 64 sampli)
- 32-głosowa polifonia z pula głosow (voice stealing)
- 4 niezalezne sloty (A/B/C/D), kazdy z wlasnym patchem
- Graf modulow oparty na DAG (topological sort, lock-free real-time swap)
- Trzy typy sygnalu: audio (zolty), CV/control (niebieski), gate/logic (pomaranczowy)
- Import patchy w formacie `.pch2` (przez g2ools Python bridge)
- Export jako VST3, AU, Standalone (JUCE 8)
- Interfejs graficzny odwzorowujacy fizyczny panel G2 z kablami Beziera
- Arpeggiator i wbudowany sekwencer
- Zarzadzanie presetami

---

## Architektura

```
HOST (DAW / Standalone)
        |
        | Audio Callback (96 kHz, bloki 64 sampli)
        v
+─────────────────────────────────────────────────────+
│                   G2 ENGINE CORE                     │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │
│  │   Slot A    │  │   Slot B    │  │  Slot C/D   │  │
│  │  Patch DAG  │  │  Patch DAG  │  │  Patch DAG  │  │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  │
│         └────────────────┴─────────────────┘         │
│                  VoicePool (32 glosy)                │
│  ┌─────────────────────────────────────────────────┐ │
│  │         ModuleRegistry (50+ modulow)             │ │
│  └─────────────────────────────────────────────────┘ │
+─────────────────────────────────────────────────────+
```

### Stos technologiczny

| Warstwa        | Technologia          | Uzasadnienie                          |
|----------------|----------------------|---------------------------------------|
| Jezyk          | C++20                | Deterministyczny RT, brak GC, SIMD    |
| Audio / Plugin | JUCE 8               | VST3 / AU / Standalone w jednym       |
| Graf modulow   | Custom DAG           | Kahn topo-sort, lock-free swap        |
| Format patcha  | JSON (nlohmann)      | Czytelny + bridge do .pch2            |
| Testy          | Catch2               | Analiza widmowa, testy timingu        |
| Build          | CMake 3.25 + CPM     | Jedno polecenie, dep auto-pobierane   |

### Graf modulow (DAG)

Kazda zmiana patcha wyzwala przeliczenie kolejnosci przetwarzania:

- Sortowanie topologiczne (algorytm Kahna) — O(V+E)
- Wykrywanie cykli → blad + komunikat uzytkownika
- Lock-free double-buffer swap dla real-time safety
- Brak alokacji pamieci w watku audio
- Deterministyczny czas wykonania

### Kluczowe wymagania silnika

- Brak alokacji pamieci w audio thread
- Lock-free komunikacja z UI thread
- Deterministyczny czas wykonania na kazdy blok

---

## Moduly

### Oscylatory

| Modul | Algorytm | Specyfika G2 |
|-------|----------|--------------|
| OscA  | PolyBLEP multi-waveform | Sine, Tri, Saw, Pulse + PWM |
| OscB  | Jak OscA + hard sync | Slave oscillator |
| OscC  | Wavetable 256-point, mipmapping, interpolacja Lanczos | Bandlimited |
| OscD  | FM (ratio-based, feedback FM) | Ratio, Index, Feedback |
| Noise | White / Pink / Red szum | — |

**Algorytm PolyBLEP (eliminacja aliasingu):**

Bandlimited Step Function dla pily i prostokata. Eliminuje aliasing bez kosztownego oversamplingu. Kazda tabela wavetable: 256 sampli x 512 harmonicznych wariantow (mipmapping), interpolacja 8-punktowa Lanczos przy playback.

### Filtry

| Modul    | Algorytm | Specyfika G2 |
|----------|----------|--------------|
| FltLP    | ZDF Ladder 4-pole | Moog-style, samoosc. przy res=4 |
| FltNord  | ZDF State-Variable | 2/4-pole LP/HP/BP/Notch, 12/24 dB |
| FltVoice | Formant filter | 5 samoglosek, morph po CV |
| FltPhase | Allpass phaser | 2/4/6/8 sekcji |

**Filtr drabinkowy ZDF (Moog-style):**

Implementacja wg Zavalishin "The Art of VA Filter Design" §6.5.
4 stany one-pole sekcji, LUT dla nieliniowosci tanh (4096 punktow),
zero-delay feedback — brak opoznienia w petli sprzezenia.

### Koperty i LFO

| Modul     | Algorytm | Opis |
|-----------|----------|------|
| EnvADSR   | Krzywe wykladnicze | Attack/Decay/Sustain/Release, G2-style timing |
| EnvAD     | One-shot | Attack/Decay |
| EnvFollow | RMS follower | Sledzenie obwiedni sygnalu audio |
| LfoA      | Wavetable LFO | Sine/Tri/Saw/Square/S&H, sync do tempa, retrigger |
| LfoB      | Jak LfoA | + konfigurowalny faz startowy |

G2 uzywa krzywych wykladniczych dla kopert — bardziej muzyczne niz liniowe.

### Efekty

| Modul        | Algorytm | Parametry |
|--------------|----------|-----------|
| FxReverb     | Schroeder / Freeverb z metalowymi allpass | size / damp / mix |
| FxDelay      | Interpolated delay line | tempo sync, feedback, stereo |
| FxChorus     | Multi-tap modulated delay | rate, depth, mix |
| FxDistort    | Waveshaping | softclip / hardclip / foldback |
| FxCompressor | Peak/RMS, feed-forward/back | ratio / thresh / attack / release |
| FxEQ         | 3-band EQ | low shelf, mid peak, high shelf |

### Mikser i routing

| Modul   | Opis |
|---------|------|
| Mix4to1 | 4 wejscia -> 1 wyjscie, poziomy A+B+C+D |
| Mix8to1 | 8 wejsc -> 1 wyjscie |
| VCA     | Wzmacniacz sterowany CV (bramka glosow) |
| Pan     | Panorama L/R sterowana CV, linear/equal-power |
| Out     | Stereo wyjscie z poziomem |

### Sekwencer i logika

| Modul   | Opis |
|---------|------|
| SeqNote | 16/32-step sekwencer melodyczny z glide |
| SeqCtrl | 16-step sekwencer CV (bipolarny) |
| ClkGen  | Wewnetrzny zegar BPM |
| ClkDiv  | Podzielnik zegara (/2 /4 /8 /16 /32) |

### MIDI -> CV

| Modul    | Opis |
|----------|------|
| KeyNote  | MIDI -> Note CV + Gate |
| KeyVelo  | MIDI -> Velocity CV |
| KeyAfter | MIDI -> Aftertouch CV |
| PitchBnd | MIDI -> Pitch Bend CV |
| ModWheel | MIDI -> Mod Wheel CV |

---

## Interfejs uzytkownika

Filozofia: interfejs odwzorowuje fizyczny panel hardware G2, nie typowe okno DAW.

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
║  │           ════════ kabel audio (zolty) ════════                   │  ║
║  └────────────────────────────────────────────────────────────────────┘  ║
║                                                                          ║
║  ┌─ PERFORMANCE ─────────────────────────────────────────────────────┐  ║
║  │  [A] [B] [C] [D]    VOL: ◎    CPU [████░░] 43%  Voices: 12/32    │  ║
║  └────────────────────────────────────────────────────────────────────┘  ║
╚══════════════════════════════════════════════════════════════════════════╝
```

### Zasady UI

- **Tlo:** ciemnoszare (#1a1a1a) — identyczny kontrast jak hardware G2
- **Moduly jako karty:** zaokraglone rogi, lekki cien, kolor kategorii:
  - Oscylatory → bordowy (#8B1A1A)
  - Filtry → ciemnoniebieski (#1A3A6B)
  - Koperty/LFO → ciemnozielony (#1A5C2A)
  - Efekty → ciemnofioletowy (#3A1A6B)
  - Mikser/Out → grafitowy (#3A3A3A)
- **Kable:** zolte (audio), niebieskie (CV), pomaranczowe (gate)
- **Knobs:** 270° zakres, drag gore/dol zmienia wartosc, Shift = fine control, double-click = reset

### Komponenty UI

| Komponent        | Opis |
|------------------|------|
| PanelView        | Glowny ciemnoszary panel G2 |
| ModuleView       | Karta modulu z knobami i portami |
| KnobView         | Pokretlo 270°, right-click menu, tooltip |
| CableView        | Kable jako krzywe Beziera ze zwisem |
| PortView         | Gniazdko kabla (kliknij i przeciagnij) |
| SlotTabView      | Zakladki A/B/C/D |
| KeyboardView     | Klawiatura MIDI inline |
| SequencerView    | Widok 16-step sekwencera |
| PresetManager    | Zarzadzanie presetami |

### Rysowanie kabli

Kable renderowane jako krzywe Beziera z naturalnym zwisem (symulacja grawitacji):
- Punkt kontrolny przesunieto w dol wzgledem wyzszego portu
- Grubosc linii: 2.5px, zaokraglone koncowki
- Alfa: 0.85 (lekka przezroczystosc przy wielu kablach)

---

## Budowanie

### Wymagania

- CMake >= 3.25
- Kompilator z obsuga C++20 (GCC 12+, Clang 15+)
- Linux: pakiety `libasound2-dev`, `libgtk-3-dev`, `libwebkit2gtk-4.1-dev`

### Linux (Ubuntu/Debian) — testowane

```bash
sudo apt install libasound2-dev libgtk-3-dev libwebkit2gtk-4.1-dev \
                 libfreetype6-dev libx11-dev libxext-dev libxrandr-dev \
                 libxcomposite-dev libxcursor-dev libxinerama-dev

git clone https://github.com/TWOJ_LOGIN/synthapp.git
cd synthapp
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### macOS i Windows — nietestowane

Projekt uzywa JUCE 8 i CMake, ktore teoretycznie wspieraja macOS i Windows,
jednak aplikacja byla rozwijana i testowana wylacznie na Linux.
Kompilacja na innych systemach moze wymagac dodatkowych poprawek.

### Wyniki budowania

```
build/
  SynthApp_artefacts/
    Release/
      VST3/       SynthApp G2.vst3      <- wtyczka VST3
      AU/         SynthApp G2.component <- wtyczka AU (macOS)
      Standalone/ SynthApp G2           <- aplikacja standalone
```

---

## Uruchamianie

**Standalone:**
```bash
./build/SynthApp_artefacts/Release/Standalone/SynthApp\ G2
```

**VST3** — skopiuj plik `.vst3` do katalogu wtyczek swojego DAW:
- Linux: `~/.vst3/`
- macOS: `~/Library/Audio/Plug-Ins/VST3/`
- Windows: `C:\Program Files\Common Files\VST3\`

---

## Testy i walidacja brzmienia

### Uruchamianie testow

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
cd build && ctest --output-on-failure
```

### Zakres testow

| Test | Metoda | Kryterium sukcesu |
|------|--------|-------------------|
| `test_osc_aliasing`    | FFT, analiza widmowa PolyBLEP | SINAD > 90 dB |
| `test_filter_response` | FFT sweep, porownanie z G2 recordings | < 0.5 dB bledu do 20 kHz |
| `test_envelope_timing` | Oscyloskop czasowy | < 1 ms odchylki |
| `test_graph_topo`      | Poprawnosc DAG i wykrywanie cykli | 100% poprawny sort |
| `test_effects`         | Testy efektow DSP | Brak artefaktow |
| `test_sequencers`      | Testy sekwencerow | Poprawny timing |
| `test_patch_format`    | Import/export JSON i .pch2 | Roundtrip bez straty |

### Walidacja brzmienia ABX

Cel: > 75% zgodnosci brzmienia z oryginalnym G2.

```
tests/
├── dsp/
│   ├── test_ladder_filter.cpp   # ZDF filter accuracy
│   ├── test_polyblep_osc.cpp    # Aliasing measurement
│   ├── test_envelope_timing.cpp # ms-accurate timing
│   └── test_graph_ordering.cpp  # DAG correctness
├── audio_references/
│   └── g2_recordings/           # Reference WAV files
└── abx/
    └── abx_runner.py            # Automated ABX comparison
```

### Checklist wydania

**Performance:**
- [ ] 32 glosy przy zlozonym patchu < 20% CPU (single-core)
- [ ] Latencja audio: <= 5 ms przy block size 64
- [ ] Brak alokacji w audio thread (sprawdzone profilerem)
- [ ] Brak wyścigów danych (ThreadSanitizer clean)

**Brzmienie:**
- [ ] FltLP: blad odpowiedzi < 0.5 dB vs. reference G2 recording
- [ ] OscA sawtooth: SINAD > 90 dB (brak aliasingu)
- [ ] ADSR timing: <= 1 ms odchylka od zadanego czasu
- [ ] ABX test: > 75% zgodnosci z brzmieniem G2

**Stabilnosc:**
- [ ] 24h stress test (losowe nuty, parametry)
- [ ] Import 20 roznych patchy .pch2 bez bledu
- [ ] Plug-in scan w Ableton/Reaper/Logic
- [ ] Brak memory leaks (Valgrind clean)

---

## Import patchy .pch2

Wymaga Pythona 3 i biblioteki `g2ools`:

```bash
pip install g2ools
python tools/pch2_to_json.py moj_patch.pch2 > moj_patch.json
```

Plik JSON mozna nastepnie zaladowac z poziomu aplikacji (File -> Open Patch).

### Format patcha (JSON)

```json
{
  "version": "G2-compatible",
  "slots": [{
    "modules": [
      {"id": 1, "type": "OscA", "params": {"waveform": 2, "pitch": 64}},
      {"id": 2, "type": "FltLP", "params": {"cutoff": 80, "resonance": 50}}
    ],
    "cables": [
      {"from": {"module": 1, "port": "audio_out"},
       "to":   {"module": 2, "port": "audio_in"},
       "type": "audio"}
    ]
  }]
}
```

---

## Kompatybilnosc z G2

- Import patchy `.pch2` (przez g2ools Python -> JSON bridge)
- Export do `.pch2` (dla uzytkownikow z prawdziwym G2)
- MIDI CC mapping zgodny z G2

**Co wiemy o oryginalnym G2:**
- Probkowanie: 96 kHz, 24-bit fixed-point DSP
- Polifonia: do 32 glosow (zalezna od zlozonosci patcha)
- Sloty: 4 niezalezne sloty (kazdy = osobny patch)
- Sygnaly: audio (zolte), CV/control (niebieskie), gate/logic (pomaranczowe)
- Moduly: ~120 modulow w oryginalnym systemie

---

## Harmonogram

| Faza | Tygodnie | Milestone |
|------|----------|-----------|
| 0 — Analiza | 1–2 | Format .pch2 przeanalizowany, reference recordings zebrane |
| 1 — Architektura | 3–4 | ModuleGraph dziala, pierwszy dzwiek (sinus) |
| 2a — Tier 1 Moduly | 5–6 | OscA + FltLP + ADSR = grajacy patch |
| 2b — Tier 2 Moduly | 7–9 | Wavetable, FM, wszystkie filtry |
| 2c — Tier 3 Efekty | 10–12 | Reverb, delay, sekwencer |
| 3 — Patchowanie | 9–10 | Import/export .pch2 dziala |
| 4 — UI | 11–14 | Kompletny patch editor |
| 5 — Plugin | 15–16 | VST3/AU dziala w DAW |
| 6 — Walidacja | Ciagle | ABX > 75% zgodnosci z G2 |

**Szacowany czas do MVP** (Tier 1 + podstawowe UI): ~8 tygodni
**Szacowany czas do kompletnej wersji**: ~16 tygodni

---

## Ryzyka i mitigacje

| Ryzyko | Prawdopodobienstwo | Mitigacja |
|--------|-------------------|-----------|
| Niedokładnosc filtrow vs oryginał | Wysokie | Iteracyjna kalibracja na reference recordings |
| Aliasing przy wysokich czestotliwosciach | Srednie | 4x oversampling dla krytycznych modulow |
| CPU overhead przy 32 glosach | Srednie | SIMD (AVX2), moduly obliczane blokami 64 sampli |
| Nieznane szczegoły algorytmow G2 | Wysokie | Reverse engineering z g2ools + analiza spektralna |
| Real-time safety (blokada watku audio) | Niskie | Lock-free design od poczatku, JUCE guidelines |

---

## Struktura projektu

```
synthapp/
├── CMakeLists.txt
├── cmake/CPM.cmake
├── src/
│   ├── PluginProcessor.cpp/.h        # AudioProcessor JUCE
│   ├── PluginEditor.cpp/.h           # Glowny widok UI
│   ├── engine/
│   │   ├── AudioEngine.cpp/.h        # Petla audio, bufory
│   │   ├── ModuleGraph.cpp/.h        # DAG + topological sort
│   │   ├── VoicePool.cpp/.h          # Zarzadzanie 32 glosami
│   │   ├── SlotManager.cpp/.h        # 4 sloty A/B/C/D
│   │   ├── Arpeggiator.cpp/.h        # Arpeggiator
│   │   ├── Sequencer.cpp/.h          # Sekwencer silnika
│   │   └── NoteEvent.h               # MIDI -> CV/Gate events
│   ├── modules/
│   │   ├── Module.h                  # Interfejs bazowy modulow
│   │   ├── ModuleRegistry.cpp/.h     # Fabryka (name -> Module*)
│   │   ├── oscillators/
│   │   │   ├── OscA.cpp/.h           # PolyBLEP multi-waveform
│   │   │   ├── OscB.cpp/.h           # Hard sync
│   │   │   ├── OscC.cpp/.h           # Wavetable
│   │   │   ├── OscD.cpp/.h           # FM
│   │   │   └── Noise.cpp/.h          # White/Pink/Red
│   │   ├── filters/
│   │   │   ├── FltLP.cpp/.h          # ZDF Ladder
│   │   │   ├── FltNord.cpp/.h        # ZDF State-Variable
│   │   │   ├── FltVoice.cpp/.h       # Formant
│   │   │   └── FltPhase.cpp/.h       # Phaser
│   │   ├── envelopes/
│   │   │   ├── EnvADSR.cpp/.h
│   │   │   ├── EnvAD.cpp/.h
│   │   │   └── EnvFollow.cpp/.h
│   │   ├── lfos/
│   │   │   ├── LfoA.cpp/.h
│   │   │   └── LfoB.cpp/.h
│   │   ├── effects/
│   │   │   ├── FxReverb.cpp/.h
│   │   │   ├── FxDelay.cpp/.h
│   │   │   ├── FxChorus.cpp/.h
│   │   │   ├── FxDistort.cpp/.h
│   │   │   ├── FxCompressor.cpp/.h
│   │   │   └── FxEQ.cpp/.h
│   │   ├── mixers/
│   │   │   ├── Mix4to1.cpp/.h
│   │   │   ├── Mix8to1.cpp/.h
│   │   │   ├── VCA.cpp/.h
│   │   │   ├── Pan.cpp/.h
│   │   │   └── Out.cpp/.h
│   │   ├── midi/
│   │   │   ├── KeyNote.cpp/.h        # MIDI -> CV + Gate
│   │   │   ├── KeyVelo.cpp/.h
│   │   │   ├── KeyAfter.cpp/.h
│   │   │   ├── PitchBnd.cpp/.h
│   │   │   └── ModWheel.cpp/.h
│   │   └── sequencers/
│   │       ├── SeqNote.cpp/.h
│   │       ├── SeqCtrl.cpp/.h
│   │       ├── ClkGen.cpp/.h
│   │       └── ClkDiv.cpp/.h
│   ├── patch/
│   │   ├── PatchFormat.cpp/.h        # JSON serialization / .pch2 bridge
│   │   └── CableSystem.cpp/.h        # Routing polaczen portow
│   └── ui/
│       ├── PanelView.cpp/.h          # Glowny panel G2
│       ├── ModuleView.cpp/.h         # Karta modulu z knobami
│       ├── KnobView.cpp/.h           # Pokretlo 270°, right-click menu
│       ├── CableView.cpp/.h          # Kable jako krzywe Beziera
│       ├── PortView.cpp/.h           # Gniazdko kabla
│       ├── SlotTabView.cpp/.h        # Zakladki A B C D
│       ├── KeyboardView.cpp/.h       # Klawiatura MIDI inline
│       ├── SimplePanelView.cpp/.h    # Uproszczony widok panelu
│       ├── SequencerView.cpp/.h      # Widok sekwencera
│       └── PresetManager.cpp/.h      # Zarzadzanie presetami
├── tests/
│   ├── test_osc_aliasing.cpp
│   ├── test_filter_response.cpp
│   ├── test_envelope_timing.cpp
│   ├── test_graph_topo.cpp
│   ├── test_effects.cpp
│   ├── test_sequencers.cpp
│   ├── test_patch_format.cpp
│   └── reference_audio/             # Nagrania G2 do testow ABX
└── tools/
    ├── pch2_to_json.py              # Konwerter .pch2 -> JSON (g2ools)
    └── abx_compare.py              # Automatyczne porownanie ABX
```

---

## Referencje algorytmow

| Algorytm            | Zrodlo |
|---------------------|--------|
| ZDF Ladder Filter   | Zavalishin — *The Art of VA Filter Design* (2018) |
| PolyBLEP Oscillator | Valimaki & Pakarinen (2007) |
| Wavetable mipmapping| Andrew Simper — *Wavetable Synthesis* notes |
| Schroeder Reverb    | Schroeder (1962) + Moorer extensions |
| Allpass Phaser      | Regalia (1988) |
| FM Synthesis        | Chowning (1973) — CCRMA paper |
| Exponential ADSR    | JUCE DSP documentation |
| G2 format .pch2     | g2ools GitHub (Python parser) |
| Modular synth theory| Puckette — *Theory and Technique of Electronic Music* |

---

## Licencja

MIT License — Copyright (c) 2026 Adami

---
---

# English

A modular synthesizer inspired by the Nord Modular G2 by Clavia (2002–2014).
The goal of the project is to faithfully recreate the G2 sound as a VST3/AU plugin
and standalone application, running in real time at 96 kHz / 24-bit.

---

## Table of contents

- [Features](#features)
- [Architecture](#architecture)
- [Modules](#modules)
- [User interface](#user-interface)
- [Building](#building)
- [Running](#running)
- [Tests and sound validation](#tests-and-sound-validation)
- [Importing pch2 patches](#importing-pch2-patches)
- [G2 compatibility](#g2-compatibility)
- [Roadmap](#roadmap)
- [Risks and mitigations](#risks-and-mitigations)
- [Project structure](#project-structure)
- [DSP algorithm references](#dsp-algorithm-references)
- [License](#license)

---

## Features

- DSP engine compatible with the G2 architecture (96 kHz, 64-sample blocks)
- 32-voice polyphony with voice pool (voice stealing)
- 4 independent slots (A/B/C/D), each with its own patch
- Module graph based on DAG (topological sort, lock-free real-time swap)
- Three signal types: audio (yellow), CV/control (blue), gate/logic (orange)
- Patch import in `.pch2` format (via g2ools Python bridge)
- Export as VST3, AU, Standalone (JUCE 8)
- Graphical interface modelled after the physical G2 panel with Bezier cables
- Arpeggiator and built-in sequencer
- Preset management

---

## Architecture

```
HOST (DAW / Standalone)
        |
        | Audio Callback (96 kHz, 64-sample blocks)
        v
+─────────────────────────────────────────────────────+
│                   G2 ENGINE CORE                     │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │
│  │   Slot A    │  │   Slot B    │  │  Slot C/D   │  │
│  │  Patch DAG  │  │  Patch DAG  │  │  Patch DAG  │  │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  │
│         └────────────────┴─────────────────┘         │
│                  VoicePool (32 voices)               │
│  ┌─────────────────────────────────────────────────┐ │
│  │         ModuleRegistry (50+ modules)             │ │
│  └─────────────────────────────────────────────────┘ │
+─────────────────────────────────────────────────────+
```

### Technology stack

| Layer          | Technology           | Rationale                           |
|----------------|----------------------|-------------------------------------|
| Language       | C++20                | Deterministic RT, no GC, SIMD       |
| Audio / Plugin | JUCE 8               | VST3 / AU / Standalone in one       |
| Module graph   | Custom DAG           | Kahn topo-sort, lock-free swap      |
| Patch format   | JSON (nlohmann)      | Human-readable + .pch2 bridge       |
| Tests          | Catch2               | Spectral analysis, timing tests     |
| Build          | CMake 3.25 + CPM     | Single command, deps auto-fetched   |

### Module graph (DAG)

Every patch change triggers recompilation of the processing order:

- Topological sort (Kahn's algorithm) — O(V+E)
- Cycle detection → error + user notification
- Lock-free double-buffer swap for real-time safety
- No memory allocation in the audio thread
- Deterministic execution time per block

### Engine requirements

- No memory allocation in the audio thread
- Lock-free communication with the UI thread
- Deterministic execution time per block

---

## Modules

### Oscillators

| Module | Algorithm | G2 specifics |
|--------|-----------|--------------|
| OscA   | PolyBLEP multi-waveform | Sine, Tri, Saw, Pulse + PWM |
| OscB   | Like OscA + hard sync | Slave oscillator |
| OscC   | 256-point wavetable, mipmapping, Lanczos interpolation | Bandlimited |
| OscD   | FM (ratio-based, feedback FM) | Ratio, Index, Feedback |
| Noise  | White / Pink / Red noise | — |

**PolyBLEP algorithm (aliasing elimination):**

Bandlimited Step Function for sawtooth and square waves. Eliminates aliasing without costly oversampling. Each wavetable: 256 samples x 512 harmonic variants (mipmapping), 8-point Lanczos interpolation during playback.

### Filters

| Module   | Algorithm | G2 specifics |
|----------|-----------|--------------|
| FltLP    | ZDF Ladder 4-pole | Moog-style, self-oscillates at res=4 |
| FltNord  | ZDF State-Variable | 2/4-pole LP/HP/BP/Notch, 12/24 dB |
| FltVoice | Formant filter | 5 vowels, morph via CV |
| FltPhase | Allpass phaser | 2/4/6/8 stages |

**ZDF Ladder Filter (Moog-style):**

Implementation based on Zavalishin "The Art of VA Filter Design" §6.5.
4 one-pole section states, tanh nonlinearity LUT (4096 points),
zero-delay feedback — no delay in the feedback loop.

### Envelopes and LFO

| Module    | Algorithm | Description |
|-----------|-----------|-------------|
| EnvADSR   | Exponential curves | Attack/Decay/Sustain/Release, G2-style timing |
| EnvAD     | One-shot | Attack/Decay |
| EnvFollow | RMS follower | Tracks audio signal envelope |
| LfoA      | Wavetable LFO | Sine/Tri/Saw/Square/S&H, tempo sync, retrigger |
| LfoB      | Like LfoA | + configurable start phase |

G2 uses exponential curves for envelopes — more musical than linear.

### Effects

| Module       | Algorithm | Parameters |
|--------------|-----------|------------|
| FxReverb     | Schroeder / Freeverb with metallic allpass | size / damp / mix |
| FxDelay      | Interpolated delay line | tempo sync, feedback, stereo |
| FxChorus     | Multi-tap modulated delay | rate, depth, mix |
| FxDistort    | Waveshaping | softclip / hardclip / foldback |
| FxCompressor | Peak/RMS, feed-forward/back | ratio / thresh / attack / release |
| FxEQ         | 3-band EQ | low shelf, mid peak, high shelf |

### Mixer and routing

| Module  | Description |
|---------|-------------|
| Mix4to1 | 4 inputs -> 1 output, levels A+B+C+D |
| Mix8to1 | 8 inputs -> 1 output |
| VCA     | CV-controlled amplifier (voice gate) |
| Pan     | L/R pan via CV, linear/equal-power |
| Out     | Stereo output with level |

### Sequencer and logic

| Module  | Description |
|---------|-------------|
| SeqNote | 16/32-step melodic sequencer with glide |
| SeqCtrl | 16-step CV sequencer (bipolar) |
| ClkGen  | Internal BPM clock generator |
| ClkDiv  | Clock divider (/2 /4 /8 /16 /32) |

### MIDI -> CV

| Module   | Description |
|----------|-------------|
| KeyNote  | MIDI -> Note CV + Gate |
| KeyVelo  | MIDI -> Velocity CV |
| KeyAfter | MIDI -> Aftertouch CV |
| PitchBnd | MIDI -> Pitch Bend CV |
| ModWheel | MIDI -> Mod Wheel CV |

---

## User interface

Philosophy: the interface mirrors the physical G2 hardware panel, not a typical DAW window.

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
║  │           ════════ audio cable (yellow) ════════                  │  ║
║  └────────────────────────────────────────────────────────────────────┘  ║
║                                                                          ║
║  ┌─ PERFORMANCE ─────────────────────────────────────────────────────┐  ║
║  │  [A] [B] [C] [D]    VOL: ◎    CPU [████░░] 43%  Voices: 12/32    │  ║
║  └────────────────────────────────────────────────────────────────────┘  ║
╚══════════════════════════════════════════════════════════════════════════╝
```

### UI principles

- **Background:** dark grey (#1a1a1a) — identical contrast to hardware G2
- **Modules as cards:** rounded corners, subtle shadow, category color:
  - Oscillators → dark red (#8B1A1A)
  - Filters → dark blue (#1A3A6B)
  - Envelopes/LFO → dark green (#1A5C2A)
  - Effects → dark purple (#3A1A6B)
  - Mixer/Out → graphite (#3A3A3A)
- **Cables:** yellow (audio), blue (CV), orange (gate)
- **Knobs:** 270° range, drag up/down changes value, Shift = fine control, double-click = reset

### UI components

| Component        | Description |
|------------------|-------------|
| PanelView        | Main dark G2 panel |
| ModuleView       | Module card with knobs and ports |
| KnobView         | 270° knob, right-click menu, tooltip |
| CableView        | Cables as Bezier curves with sag |
| PortView         | Cable socket (click and drag) |
| SlotTabView      | Tabs A/B/C/D |
| KeyboardView     | Inline MIDI keyboard |
| SequencerView    | 16-step sequencer view |
| PresetManager    | Preset management |

### Cable rendering

Cables rendered as Bezier curves with natural sag (gravity simulation):
- Control point shifted down relative to the higher port
- Line width: 2.5px, rounded caps
- Alpha: 0.85 (slight transparency when many cables overlap)

---

## Building

### Requirements

- CMake >= 3.25
- C++20 compiler (GCC 12+, Clang 15+)
- Linux: `libasound2-dev`, `libgtk-3-dev`, `libwebkit2gtk-4.1-dev`

### Linux (Ubuntu/Debian) — tested

```bash
sudo apt install libasound2-dev libgtk-3-dev libwebkit2gtk-4.1-dev \
                 libfreetype6-dev libx11-dev libxext-dev libxrandr-dev \
                 libxcomposite-dev libxcursor-dev libxinerama-dev

git clone https://github.com/YOUR_USERNAME/synthapp.git
cd synthapp
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### macOS and Windows — untested

The project uses JUCE 8 and CMake which theoretically support macOS and Windows,
but the application was developed and tested on Linux only.
Building on other platforms may require additional fixes.

### Build output

```
build/
  SynthApp_artefacts/
    Release/
      VST3/       SynthApp G2.vst3      <- VST3 plugin
      AU/         SynthApp G2.component <- AU plugin (macOS)
      Standalone/ SynthApp G2           <- standalone app
```

---

## Running

**Standalone:**
```bash
./build/SynthApp_artefacts/Release/Standalone/SynthApp\ G2
```

**VST3** — copy the `.vst3` file to your DAW's plugin directory:
- Linux: `~/.vst3/`
- macOS: `~/Library/Audio/Plug-Ins/VST3/`
- Windows: `C:\Program Files\Common Files\VST3\`

---

## Tests and sound validation

### Running tests

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
cd build && ctest --output-on-failure
```

### Test coverage

| Test | Method | Success criterion |
|------|--------|-------------------|
| `test_osc_aliasing`    | FFT spectral analysis | SINAD > 90 dB |
| `test_filter_response` | FFT sweep vs G2 recordings | < 0.5 dB error up to 20 kHz |
| `test_envelope_timing` | Time-domain oscilloscope | < 1 ms deviation |
| `test_graph_topo`      | DAG correctness + cycle detection | 100% correct sort |
| `test_effects`         | DSP effects tests | No artifacts |
| `test_sequencers`      | Sequencer tests | Correct timing |
| `test_patch_format`    | JSON and .pch2 import/export | Lossless roundtrip |

### ABX sound validation

Target: > 75% match with the original G2 sound.

```
tests/
├── dsp/
│   ├── test_ladder_filter.cpp   # ZDF filter accuracy
│   ├── test_polyblep_osc.cpp    # Aliasing measurement
│   ├── test_envelope_timing.cpp # ms-accurate timing
│   └── test_graph_ordering.cpp  # DAG correctness
├── audio_references/
│   └── g2_recordings/           # Reference WAV files
└── abx/
    └── abx_runner.py            # Automated ABX comparison
```

### Release checklist

**Performance:**
- [ ] 32 voices on complex patch < 20% CPU (single-core)
- [ ] Audio latency: <= 5 ms at block size 64
- [ ] No allocations in audio thread (verified with profiler)
- [ ] No data races (ThreadSanitizer clean)

**Sound:**
- [ ] FltLP: response error < 0.5 dB vs. G2 reference recording
- [ ] OscA sawtooth: SINAD > 90 dB (no aliasing)
- [ ] ADSR timing: <= 1 ms deviation from set time
- [ ] ABX test: > 75% match with G2 sound

**Stability:**
- [ ] 24h stress test (random notes, parameters)
- [ ] Import 20 different .pch2 patches without errors
- [ ] Plug-in scan in Ableton/Reaper/Logic
- [ ] No memory leaks (Valgrind clean)

---

## Importing .pch2 patches

Requires Python 3 and the `g2ools` library:

```bash
pip install g2ools
python tools/pch2_to_json.py my_patch.pch2 > my_patch.json
```

The resulting JSON file can then be loaded from within the application (File -> Open Patch).

### Patch format (JSON)

```json
{
  "version": "G2-compatible",
  "slots": [{
    "modules": [
      {"id": 1, "type": "OscA", "params": {"waveform": 2, "pitch": 64}},
      {"id": 2, "type": "FltLP", "params": {"cutoff": 80, "resonance": 50}}
    ],
    "cables": [
      {"from": {"module": 1, "port": "audio_out"},
       "to":   {"module": 2, "port": "audio_in"},
       "type": "audio"}
    ]
  }]
}
```

---

## G2 compatibility

- Import of `.pch2` patches (via g2ools Python -> JSON bridge)
- Export to `.pch2` (for users with a real G2)
- MIDI CC mapping compatible with G2

**Known facts about the original G2:**
- Sample rate: 96 kHz, 24-bit fixed-point DSP
- Polyphony: up to 32 voices (depending on patch complexity)
- Slots: 4 independent slots (each = a separate patch)
- Signals: audio (yellow), CV/control (blue), gate/logic (orange)
- Modules: ~120 modules in the original system

---

## Roadmap

| Phase | Weeks | Milestone |
|-------|-------|-----------|
| 0 — Analysis | 1–2 | .pch2 format analyzed, reference recordings collected |
| 1 — Architecture | 3–4 | ModuleGraph working, first sound (sine) |
| 2a — Tier 1 Modules | 5–6 | OscA + FltLP + ADSR = playable patch |
| 2b — Tier 2 Modules | 7–9 | Wavetable, FM, all filters |
| 2c — Tier 3 Effects | 10–12 | Reverb, delay, sequencer |
| 3 — Patching | 9–10 | .pch2 import/export working |
| 4 — UI | 11–14 | Complete patch editor |
| 5 — Plugin | 15–16 | VST3/AU working in DAW |
| 6 — Validation | Ongoing | ABX > 75% match with G2 |

**Estimated time to MVP** (Tier 1 + basic UI): ~8 weeks
**Estimated time to full version**: ~16 weeks

---

## Risks and mitigations

| Risk | Probability | Mitigation |
|------|-------------|------------|
| Filter inaccuracy vs original | High | Iterative calibration on reference recordings |
| Aliasing at high frequencies | Medium | 4x oversampling for critical modules |
| CPU overhead at 32 voices | Medium | SIMD (AVX2), modules processed in 64-sample blocks |
| Unknown G2 algorithm details | High | Reverse engineering with g2ools + spectral analysis |
| Real-time safety (audio thread lock) | Low | Lock-free design from the start, JUCE guidelines |

---

## Project structure

```
synthapp/
├── CMakeLists.txt
├── cmake/CPM.cmake
├── src/
│   ├── PluginProcessor.cpp/.h        # JUCE AudioProcessor
│   ├── PluginEditor.cpp/.h           # UI root view
│   ├── engine/
│   │   ├── AudioEngine.cpp/.h        # Audio loop, buffers
│   │   ├── ModuleGraph.cpp/.h        # DAG + topological sort
│   │   ├── VoicePool.cpp/.h          # 32-voice management
│   │   ├── SlotManager.cpp/.h        # Slots A/B/C/D
│   │   ├── Arpeggiator.cpp/.h        # Arpeggiator
│   │   ├── Sequencer.cpp/.h          # Engine sequencer
│   │   └── NoteEvent.h               # MIDI -> CV/Gate events
│   ├── modules/
│   │   ├── Module.h                  # Module base interface
│   │   ├── ModuleRegistry.cpp/.h     # Factory (name -> Module*)
│   │   ├── oscillators/
│   │   │   ├── OscA.cpp/.h           # PolyBLEP multi-waveform
│   │   │   ├── OscB.cpp/.h           # Hard sync
│   │   │   ├── OscC.cpp/.h           # Wavetable
│   │   │   ├── OscD.cpp/.h           # FM
│   │   │   └── Noise.cpp/.h          # White/Pink/Red
│   │   ├── filters/
│   │   │   ├── FltLP.cpp/.h          # ZDF Ladder
│   │   │   ├── FltNord.cpp/.h        # ZDF State-Variable
│   │   │   ├── FltVoice.cpp/.h       # Formant
│   │   │   └── FltPhase.cpp/.h       # Phaser
│   │   ├── envelopes/
│   │   │   ├── EnvADSR.cpp/.h
│   │   │   ├── EnvAD.cpp/.h
│   │   │   └── EnvFollow.cpp/.h
│   │   ├── lfos/
│   │   │   ├── LfoA.cpp/.h
│   │   │   └── LfoB.cpp/.h
│   │   ├── effects/
│   │   │   ├── FxReverb.cpp/.h
│   │   │   ├── FxDelay.cpp/.h
│   │   │   ├── FxChorus.cpp/.h
│   │   │   ├── FxDistort.cpp/.h
│   │   │   ├── FxCompressor.cpp/.h
│   │   │   └── FxEQ.cpp/.h
│   │   ├── mixers/
│   │   │   ├── Mix4to1.cpp/.h
│   │   │   ├── Mix8to1.cpp/.h
│   │   │   ├── VCA.cpp/.h
│   │   │   ├── Pan.cpp/.h
│   │   │   └── Out.cpp/.h
│   │   ├── midi/
│   │   │   ├── KeyNote.cpp/.h        # MIDI -> CV + Gate
│   │   │   ├── KeyVelo.cpp/.h
│   │   │   ├── KeyAfter.cpp/.h
│   │   │   ├── PitchBnd.cpp/.h
│   │   │   └── ModWheel.cpp/.h
│   │   └── sequencers/
│   │       ├── SeqNote.cpp/.h
│   │       ├── SeqCtrl.cpp/.h
│   │       ├── ClkGen.cpp/.h
│   │       └── ClkDiv.cpp/.h
│   ├── patch/
│   │   ├── PatchFormat.cpp/.h        # JSON serialization / .pch2 bridge
│   │   └── CableSystem.cpp/.h        # Port connection routing
│   └── ui/
│       ├── PanelView.cpp/.h          # Main G2 panel
│       ├── ModuleView.cpp/.h         # Module card with knobs
│       ├── KnobView.cpp/.h           # 270° knob, right-click menu
│       ├── CableView.cpp/.h          # Cables as Bezier curves
│       ├── PortView.cpp/.h           # Cable socket
│       ├── SlotTabView.cpp/.h        # Tabs A B C D
│       ├── KeyboardView.cpp/.h       # Inline MIDI keyboard
│       ├── SimplePanelView.cpp/.h    # Simplified panel view
│       ├── SequencerView.cpp/.h      # Sequencer view
│       └── PresetManager.cpp/.h      # Preset management
├── tests/
│   ├── test_osc_aliasing.cpp
│   ├── test_filter_response.cpp
│   ├── test_envelope_timing.cpp
│   ├── test_graph_topo.cpp
│   ├── test_effects.cpp
│   ├── test_sequencers.cpp
│   ├── test_patch_format.cpp
│   └── reference_audio/             # G2 recordings for ABX tests
└── tools/
    ├── pch2_to_json.py              # .pch2 -> JSON converter (g2ools)
    └── abx_compare.py              # Automated ABX comparison
```

---

## DSP algorithm references

| Algorithm           | Source |
|---------------------|--------|
| ZDF Ladder Filter   | Zavalishin — *The Art of VA Filter Design* (2018) |
| PolyBLEP Oscillator | Valimaki & Pakarinen (2007) |
| Wavetable mipmapping| Andrew Simper — *Wavetable Synthesis* notes |
| Schroeder Reverb    | Schroeder (1962) + Moorer extensions |
| Allpass Phaser      | Regalia (1988) |
| FM Synthesis        | Chowning (1973) — CCRMA paper |
| Exponential ADSR    | JUCE DSP documentation |
| G2 .pch2 format     | g2ools GitHub (Python parser) |
| Modular synth theory| Puckette — *Theory and Technique of Electronic Music* |

---

## License

MIT License — Copyright (c) 2026 Adami
