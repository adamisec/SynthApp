#pragma once
#include <vector>
#include <atomic>
#include <algorithm>
#include <random>

namespace synth {

// ── Arpeggiator — sample-accurate timing, wywoływany z audio thread ──────────
// Ustawienia (BPM, pattern, podział, oktawy) są atomowe — bezpieczne z UI.
// noteOn / noteOff / process() — wyłącznie z audio thread (processBlock).
class Arpeggiator {
public:
    enum class Pattern { Up = 0, Down, UpDown, Random };

    // Zdarzenie generowane przez process()
    struct Event {
        bool  noteOn;
        int   note;
        float velocity;
        int   sampleOffset;  // w obrębie bieżącego bloku
    };

    // Inicjalizacja przy zmianie sample rate (audio thread)
    void prepare(double sampleRate);

    // Informacje o wciśniętych klawiszach — wyłącznie z audio thread
    void noteOn (int note, float velocity);
    void noteOff(int note);

    // Generuje zdarzenia dla bieżącego bloku — wyłącznie z audio thread
    std::vector<Event> process(int numSamples);

    // ── Ustawienia (bezpieczne z UI thread) ──────────────────────────────────
    void setEnabled (bool e) {
        enabled_.store(e);
        if (!e) needReset_.store(true);
    }
    void setBPM(float bpm) {
        bpm_.store(std::max(40.0f, std::min(280.0f, bpm)));
    }
    void setDivision(int d)     { division_.store(d); }
    void setPattern (Pattern p) { pattern_.store(static_cast<int>(p)); }
    void setOctaves (int n)     { octaves_.store(std::max(1, std::min(3, n))); }

    bool    isEnabled()   const { return enabled_.load(); }
    float   getBPM()      const { return bpm_.load(); }
    int     getDivision() const { return division_.load(); }
    Pattern getPattern()  const { return static_cast<Pattern>(pattern_.load()); }
    int     getOctaves()  const { return octaves_.load(); }

private:
    // Atomowe ustawienia (UI thread pisze, audio thread czyta)
    std::atomic<bool>  enabled_   { false };
    std::atomic<float> bpm_       { 120.0f };
    std::atomic<int>   division_  { 16 };      // 4=ćwierćnuta, 8=ósemka, …
    std::atomic<int>   pattern_   { 0 };
    std::atomic<int>   octaves_   { 1 };
    std::atomic<bool>  needReset_ { false };

    // Stan wyłącznie w audio thread
    double sampleRate_  = 96000.0;
    int    posInStep_   = 0;      // pozycja w bieżącym kroku [0..stepLen-1]
    int    stepIndex_   = 0;      // który dźwięk w arpNotes_
    int    currentNote_ = -1;     // aktualnie grany dźwięk (-1 = cisza)
    float  velocity_    = 0.8f;
    bool   ascending_   = true;   // kierunek dla UpDown
    bool   gateOpen_    = false;

    std::vector<int> heldNotes_;  // klawisze trzymane przez użytkownika
    std::vector<int> arpNotes_;   // rozwinięte na oktawy

    std::mt19937 rng_ { 42 };

    void   rebuildArpNotes();
    void   advanceStep();
    double samplesPerStep() const;

    static constexpr float kGateRatio = 0.72f;  // 72% kroku = dźwięk
};

} // namespace synth
