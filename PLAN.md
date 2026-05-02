# SynthApp — Plan Odtworzenia Brzmienia Nord Modular G2

## Cel Projektu

Stworzyć aplikację DSP, która **wiernie odtworzy brzmienie Nord Modular G2** —
cyfrowego syntezatora modularnego firmy Clavia (2002–2014), działającego na 96 kHz / 24-bit.

---

## Faza 0 — Analiza Źródłowa (Tydzień 1–2)

### Co wiemy o G2
- **Próbkowanie:** 96 kHz, 24-bit fixed-point DSP
- **Polifonia:** do 32 głosów (zależnie od złożoności patcha)
- **Sloty:** 4 niezależne sloty (każdy = osobny patch)
- **Sygnały:** audio (żółte), CV/control (niebieskie), gate/logic (pomarańczowe)
- **Moduły:** ~120 modułów w oryginalnym systemie
- **Znane projekty OS:** g2ools (Python), nm2orc (Csound), G2 Editor (Java)

### Działania
- [ ] Pobrać i przeanalizować format pliku patchy `.pch2` (g2ools parser)
- [ ] Przeanalizować kompletną listę modułów z dokumentacji G2 Editor
- [ ] Zmierzyć i porównać odpowiedź częstotliwościową filtrów G2 (reference recordings)
- [ ] Zebrać referencyjne nagrania brzmień G2 do testów ABX

---

## Faza 1 — Architektura Silnika DSP (Tydzień 3–4)

### Wybór Technologii

| Warstwa | Technologia | Uzasadnienie |
|---|---|---|
| Silnik DSP | **C++17** | Deterministyczny, brak GC pauz, SIMD |
| Audio API | **JUCE 7** | Cross-platform, VST3/AU/AAX plugin |
| Graf modułów | **Custom DAG** | Topological sort → real-time safe |
| UI | **JUCE + OpenGL** | Natywna wydajność, patch cables rendering |
| Testy | **Catch2 + FFTW** | Analiza widmowa, testy błędu fazowego |

### Architektura Systemu

```
┌─────────────────────────────────────────────────────┐
│                   HOST (DAW / Standalone)            │
└──────────────────────┬──────────────────────────────┘
                       │ Audio Callback (96kHz, 64-sample blocks)
┌──────────────────────▼──────────────────────────────┐
│                  G2 ENGINE CORE                      │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │
│  │   Slot 1    │  │   Slot 2    │  │   Slot 3/4  │  │
│  │  Patch DAG  │  │  Patch DAG  │  │  Patch DAG  │  │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  │
│         └────────────────┴─────────────────┘         │
│                    Voice Pool (32 voices)             │
│  ┌─────────────────────────────────────────────────┐ │
│  │            Module Registry (120+ modules)        │ │
│  └─────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────┘
```

### Graf Modułów (DAG)

```cpp
class ModuleGraph {
    // Topological sort przy każdej zmianie patcha
    // Lock-free swap double-buffer do real-time safety
    // Każdy moduł: process(AudioBuffer& audio, CVBuffer& cv, int numSamples)
};
```

**Kluczowe wymagania:**
- Brak alokacji pamięci w audio thread
- Lock-free komunikacja z UI thread
- Deterministyczny czas wykonania

---

## Faza 2 — Implementacja Modułów (Tydzień 5–12)

### Priorytety Modułów (kolejność implementacji)

#### Tier 1 — Fundament (tydzień 5–6)
| Moduł | Algorytm | Specyfika G2 |
|---|---|---|
| **OscA** | BandLimited sawtooth (PolyBLEP) | Sine, Tri, Saw, Pulse + PWM |
| **OscB** | Hard sync capable | Slave oscillator |
| **FltLP** | Zero-delay feedback ladder | Moog-style, 4-pole |
| **FltNord** | Multimode state-variable | 12/24 dB LP/HP/BP/Notch |
| **EnvADSR** | Exponential curves | Precyzyjny G2-style timing |
| **LfoA** | Wavetable LFO | Sync do tempa, retrigger |

#### Tier 2 — Rozszerzenie (tydzień 7–9)
| Moduł | Algorytm |
|---|---|
| **OscC** | Wavetable (256-point, bandlimited interpolation) |
| **OscD** | FM (ratio-based, feedback FM) |
| **FltHP/BP/Notch** | ZDF state-variable |
| **FltVoice** | Formant filter (vowel synthesis) |
| **FltPhase** | 2/4/6/8-stage allpass phaser |
| **EnvAD/EnvFollower** | Attack-Decay + RMS follower |
| **Mix2-1, Mix4-1, Mix8-1** | Weighted sum |
| **Crossfader/Pan** | Linear/equal-power |

#### Tier 3 — Efekty i Logika (tydzień 10–12)
| Moduł | Algorytm |
|---|---|
| **FxReverb** | Schroeder/Freeverb z metalowymi allpass |
| **FxDelay** | Interpolated delay line (tempo sync) |
| **FxChorus** | Multi-tap modulated delay |
| **FxFlanger** | Short delay + feedback |
| **Compressor** | Peak/RMS, feed-forward/back |
| **Distortion** | Waveshaping (soft clip, hard clip, foldback) |
| **SeqNote** | 16/32-step sequencer z glide |
| **SeqCtrl** | CV sequencer |
| **LogicAnd/Or/Xor** | Gate logic |

### Kluczowe Algorytmy DSP

#### Filtr drabinkowy (Moog-style, ZDF)
```cpp
// Zero-Delay Feedback Ladder Filter
// Źródło: Zavalishin "The Art of VA Filter Design"
class LadderFilter {
    double s[4] = {};           // stany (4 one-pole sekcje)
    double tanh_table[4096];    // LUT dla nieliniowości

    float process(float x, float cutoff, float resonance) {
        double g = tan(M_PI * cutoff / sampleRate);
        double k = 4.0 * resonance;
        // ZDF rozwiązanie układu równań...
    }
};
```

#### Oscylator PolyBLEP (bez aliasingu)
```cpp
// Bandlimited Step Function dla piły i prostokąta
// Eliminuje aliasing bez kosztownego oversamplingu
double polyblep(double t, double dt);
```

#### Wavetable z interpolacją sinc
```cpp
// Każda tabela: 256 sampli × 512 harmonicznych wariantów (mipmapping)
// Interpolacja 8-punktowa Lanczos przy playback
```

---

## Faza 3 — System Patchowania (Tydzień 9–10)

### Format Patcha (kompatybilny z .pch2)

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

### Graf Topologiczny
- Sortowanie topologiczne (Kahn's algorithm) przy każdej zmianie
- Wykrywanie cykli → błąd + komunikat użytkownika
- Optymalizacja kolejności dla cache locality

---

## Faza 4 — Interfejs Użytkownika (Tydzień 11–14)

### Layout (wzorowany na G2 Editor)

```
┌──────────────────────────────────────────────────────────┐
│  TOOLBAR: [New][Load][Save][Slot 1▼] BPM:[120] [Play]    │
├──────────────────────────────────────────────────────────┤
│                                                          │
│  MODULE BROWSER          PATCH CANVAS                    │
│  ┌──────────────┐        ┌────────────────────────────┐  │
│  │ Oscillators  │        │  [OscA]───►[FltLP]───►[Out]│  │
│  │ Filters      │        │     │                      │  │
│  │ Envelopes    │        │  [ADSR]──►[FltLP.cutoff]   │  │
│  │ LFOs         │        │                            │  │
│  │ Effects      │        │  (kable: audio=żółty,      │  │
│  │ Sequencers   │        │         cv=niebieski,      │  │
│  │ Logic        │        │         gate=pomarańczowy) │  │
│  └──────────────┘        └────────────────────────────┘  │
├──────────────────────────────────────────────────────────┤
│  PERFORMANCE: CPU [████░░] 43%  Voices: 12/32            │
└──────────────────────────────────────────────────────────┘
```

### Wymagania UI
- Przeciągnij-i-upuść moduły na canvas
- Rysowanie kabli myszą (port → port)
- Zoom / pan na canvas (duże patche)
- Podgląd sygnału na każdym kablu (oscyloskop inline)
- Dark mode (oryginalny styl G2: ciemnoszary + kolor kabli)

---

## Faza 5 — Plugin i Integracje (Tydzień 15–16)

### Formaty eksportu
- **VST3** (Windows/Linux) — priorytet
- **AU** (macOS)
- **Standalone** (JUCE AudioDeviceManager)
- **CLAP** (opcjonalnie)

### Kompatybilność z G2
- Import patchy `.pch2` (przez g2ools Python → JSON bridge)
- Export do `.pch2` (dla użytkowników z prawdziwym G2)
- MIDI CC mapping (zgodny z G2)

---

## Faza 6 — Walidacja Brzmienia (Ciągłe)

### Metodologia testów

| Test | Metoda | Kryterium sukcesu |
|---|---|---|
| Odpowiedź filtrów | FFT sweep, porównanie z G2 recordings | < 0.5 dB błąd do 20kHz |
| Aliasing oscylatorów | Analiza widmowa PolyBLEP | SINAD > 90 dB |
| Timing envelop | Oscyloskop czasowy | < 1ms odchyłka |
| CPU performance | Profiler (64-voice patch) | < 20% CPU single-core |
| ABX test brzmienia | Blind test z G2 reference | > 80% zgodność |

### Narzędzia testowe
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

---

## Struktura Projektu

```
synthapp/
├── PLAN.md                    # Ten dokument
├── CMakeLists.txt
├── src/
│   ├── engine/
│   │   ├── ModuleGraph.cpp    # DAG + topological sort
│   │   ├── VoicePool.cpp      # Polyphony management
│   │   ├── SlotManager.cpp    # 4 sloty G2
│   │   └── AudioEngine.cpp    # JUCE AudioProcessor
│   ├── modules/
│   │   ├── oscillators/
│   │   │   ├── OscA.cpp       # PolyBLEP multi-waveform
│   │   │   ├── OscB.cpp       # Hard sync
│   │   │   ├── OscC.cpp       # Wavetable
│   │   │   └── OscD.cpp       # FM
│   │   ├── filters/
│   │   │   ├── FltLP.cpp      # ZDF Ladder
│   │   │   ├── FltNord.cpp    # ZDF State-variable
│   │   │   ├── FltVoice.cpp   # Formant
│   │   │   └── FltPhase.cpp   # Phaser
│   │   ├── envelopes/
│   │   ├── lfos/
│   │   ├── effects/
│   │   ├── sequencers/
│   │   └── logic/
│   ├── patch/
│   │   ├── PatchFormat.cpp    # .pch2 parser/serializer
│   │   └── CableSystem.cpp    # Cable routing
│   └── ui/
│       ├── PatchCanvas.cpp    # OpenGL canvas
│       ├── ModuleBrowser.cpp
│       └── CableRenderer.cpp
├── tests/
└── tools/
    └── g2ools_bridge.py       # Python bridge dla .pch2
```

---

## Harmonogram

| Faza | Tygodnie | Milestone |
|---|---|---|
| 0 — Analiza | 1–2 | Format .pch2 przeanalizowany, reference recordings zebrane |
| 1 — Architektura | 3–4 | ModuleGraph działa, pierwszy dźwięk (sinus) |
| 2a — Tier 1 Moduły | 5–6 | OscA + FltLP + ADSR = grający patch |
| 2b — Tier 2 Moduły | 7–9 | Wavetable, FM, wszystkie filtry |
| 2c — Tier 3 Efekty | 10–12 | Reverb, delay, sequencer |
| 3 — Patchowanie | 9–10 | Import/export .pch2 działa |
| 4 — UI | 11–14 | Kompletny patch editor |
| 5 — Plugin | 15–16 | VST3/AU działa w DAW |
| 6 — Walidacja | Ciągłe | ABX > 80% zgodności z G2 |

**Szacowany czas do MVP (Tier 1 + podstawowe UI):** ~8 tygodni (2 programistów DSP)
**Szacowany czas do kompletnej wersji:** ~16 tygodni

---

## Ryzyka i Mitigacje

| Ryzyko | Prawdopodobieństwo | Mitigacja |
|---|---|---|
| Niedokładność filtrów vs oryginał | Wysokie | Iteracyjna kalibracja na reference recordings |
| Aliasing przy wysokich częstotliwościach | Średnie | 4x oversampling dla krytycznych modułów |
| CPU overhead przy 32 głosach | Średnie | SIMD (AVX2), moduły obliczone blok 64-sample |
| Nieznane szczegóły algorytmów G2 | Wysokie | Reverse engineering z g2ools + analiza spektralna |
| Real-time safety (blokada wątku audio) | Niskie | Lock-free design od początku, JUCE guidelines |

---

## Referencje Techniczne

- **Algorytmy DSP:** Zavalishin — *The Art of VA Filter Design* (2018, darmowy PDF)
- **PolyBLEP:** Valimaki & Pakarinen — *Antialiasing Oscillators in Subtractive Synthesis*
- **Wavetable:** Andrew Simper — wavetable synthesis notes
- **G2 format:** g2ools GitHub (Python parser dla .pch2)
- **JUCE DSP:** JUCE DSP Module documentation
- **Modular synth theory:** Puckette — *Theory and Technique of Electronic Music*
