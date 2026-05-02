#pragma once
#include <cstdint>

namespace synth {

struct NoteEvent {
    enum class Type : uint8_t { NoteOn, NoteOff, PitchBend, ModWheel, Aftertouch };

    Type    type;
    int     sampleOffset;  // pozycja w bloku

    uint8_t note;          // MIDI 0–127
    uint8_t velocity;      // 0–127
    float   value;         // dla PitchBend/ModWheel/Aftertouch: -1..1 lub 0..1
    int     voice;         // -1 = broadcast do wszystkich głosów
};

} // namespace synth
