#pragma once
#include <array>
#include <vector>
#include <atomic>
#include <algorithm>
#include <cmath>

namespace synth {

// ── Sequencer — sample-accurate 16-krokowy sekwencer melodyczny ───────────────
// Generuje zdarzenia NoteOn/Off z zaprogramowanych kroków.
// Ustawienia kroków i BPM są atomowe — bezpieczne z UI thread.
// process() wywoływany wyłącznie z audio thread (processBlock).
class Sequencer {
public:
    static constexpr int kMaxSteps = 16;

    struct Event {
        bool  noteOn;
        int   note;
        float velocity;
        int   sampleOffset;
    };

    Sequencer();

    void prepare(double sampleRate);
    std::vector<Event> process(int numSamples);

    // ── Ustawienia (bezpieczne z UI thread) ───────────────────────────────────
    void setEnabled (bool e)  { enabled_.store(e); if (!e) needReset_.store(true); }
    void setBPM     (float b) { bpm_.store(std::max(40.0f, std::min(280.0f, b))); }
    void setDivision(int d)   { division_.store(d); }
    void setNumSteps(int n)   { numSteps_.store(std::max(1, std::min(kMaxSteps, n))); }

    void setStepNote  (int i, int note)  { if (i >= 0 && i < kMaxSteps) stepNote_[i].store(note); }
    void setStepActive(int i, bool a)    { if (i >= 0 && i < kMaxSteps) stepActive_[i].store(a); }

    bool  isEnabled()  const { return enabled_.load(); }
    float getBPM()     const { return bpm_.load(); }
    int   getDivision()const { return division_.load(); }
    int   getNumSteps()const { return numSteps_.load(); }
    int   currentStep()const { return currentStep_.load(); }
    int   getStepNote  (int i) const { return (i >= 0 && i < kMaxSteps) ? stepNote_[i].load()   : 60;   }
    bool  getStepActive(int i) const { return (i >= 0 && i < kMaxSteps) ? stepActive_[i].load() : false; }

private:
    std::atomic<bool>  enabled_      { false };
    std::atomic<float> bpm_          { 120.0f };
    std::atomic<int>   division_     { 16 };
    std::atomic<int>   numSteps_     { 8 };
    std::atomic<bool>  needReset_    { false };
    std::atomic<int>   currentStep_  { 0 };   // tylko do odczytu w UI

    std::array<std::atomic<int>,  kMaxSteps> stepNote_;
    std::array<std::atomic<bool>, kMaxSteps> stepActive_;

    double sampleRate_  = 96000.0;
    int    posInStep_   = 0;
    int    step_        = -1;  // -1 = przed pierwszym krokiem
    int    currentNote_ = -1;
    bool   gateOpen_    = false;

    double samplesPerStep() const;
    static constexpr float kGateRatio = 0.72f;
};

} // namespace synth
