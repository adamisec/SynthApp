#pragma once
#include "ModuleGraph.h"
#include "NoteEvent.h"
#include <array>
#include <memory>
#include <functional>

namespace synth {

// ── Jeden głos polifoniczny ───────────────────────────────────────────────────
// Każdy głos ma własną kopię grafu modułów (własny stan DSP).
struct Voice {
    static constexpr int kInactive = -1;

    int   note       = kInactive;  // aktywna nuta; kInactive = wolny / w release
    int   voiceIdx   = 0;
    float velocity   = 0.0f;
    bool  sustained  = false;   // trzymany przez pedał sustain
    bool  releasing  = false;   // gate off, ale Release jeszcze biegnie

    ModuleGraph graph;

    // Głos jest przetwarzany jeśli gra nutę LUB jest w fazie Release
    bool isActive() const { return note != kInactive || releasing; }
    bool isFree()   const { return !isActive(); }
};

// ── Pula głosów (32 głosy jak w G2) ──────────────────────────────────────────
class VoicePool {
public:
    static constexpr int kMaxVoices = 32;

    // Fabryka grafu wywoływana przy tworzeniu każdego głosu
    // Powinna dodać moduły i kable do przekazanego ModuleGraph
    using GraphFactory = std::function<void(ModuleGraph&)>;

    void prepare(double sampleRate, int blockSize, GraphFactory factory);

    // Obsługa zdarzeń MIDI — bezpieczne z audio thread
    void noteOn (int note, float velocity, int sampleOffset);
    void noteOff(int note, int sampleOffset);

    // Przetwarzanie wszystkich aktywnych głosów, sumowanie do outputu
    // outputL/outputR: bufory stereo do których dodajemy (+=)
    void process(float* outputL, float* outputR, int numSamples);

    int          activeVoiceCount() const;
    ModuleGraph& voiceGraph(int idx) { return voices_[idx].graph; }

    // ID modułu Out w grafie głosu — ustawiany przez AudioEngine po prepare()
    void setOutModuleId(int id) { outModuleId_ = id; }

    // Propagacja globalnych zdarzeń MIDI do wszystkich głosów.
    // moduleId = ID modułu w grafie (np. PitchBnd, ModWheel, KeyAfter).
    // Wartość 0.0 = min, 0.5 = środek, 1.0 = max.
    void setGlobalModuleParam(int moduleId, int paramIdx, float value);

    // Convenience methods (wywołuj z processBlock)
    void setPitchBendModuleId  (int id) { pitchBndModuleId_   = id; }
    void setModWheelModuleId   (int id) { modWheelModuleId_   = id; }
    void setAftertouchModuleId (int id) { aftertouchModuleId_ = id; }

    void setPitchBend  (float v01) { if (pitchBndModuleId_   >= 0) setGlobalModuleParam(pitchBndModuleId_,   0, v01); }
    void setModWheel   (float v01) { if (modWheelModuleId_   >= 0) setGlobalModuleParam(modWheelModuleId_,   0, v01); }
    void setAftertouch (float v01) { if (aftertouchModuleId_ >= 0) setGlobalModuleParam(aftertouchModuleId_, 0, v01); }
    void setSustain    (bool held);   // CC64 — trzyma głosy podczas noteOff

private:
    // Znajdź wolny głos lub najstarszy (voice stealing)
    Voice* allocateVoice(int note);
    Voice* findVoice(int note);  // głos grający daną nutę

    std::array<Voice, kMaxVoices> voices_;

    double      sampleRate_ = 96000.0;
    int         blockSize_  = 64;
    GraphFactory factory_;

    // Licznik do voice stealing (najstarszy głos = najniższy timestamp)
    uint32_t voiceAge_[kMaxVoices] = {};
    uint32_t ageClock_ = 0;

    bool sustainHeld_      = false;
    int outModuleId_       = 5;   // ustawiany przez AudioEngine
    int pitchBndModuleId_  = -1;  // -1 = brak w bieżącym patchu
    int modWheelModuleId_  = -1;
    int aftertouchModuleId_= -1;
};

} // namespace synth
