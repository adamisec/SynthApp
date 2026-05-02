#include "Arpeggiator.h"
#include <cmath>

namespace synth {

void Arpeggiator::prepare(double sampleRate) {
    sampleRate_ = sampleRate;
    posInStep_  = 0;
    stepIndex_  = 0;
    currentNote_ = -1;
    gateOpen_   = false;
    ascending_  = true;
    heldNotes_.clear();
    arpNotes_.clear();
}

// ── Zarządzanie trzymanymi nutami ─────────────────────────────────────────────

void Arpeggiator::noteOn(int note, float velocity) {
    // Dodaj (bez duplikatów), zachowaj sortowanie
    if (std::find(heldNotes_.begin(), heldNotes_.end(), note) == heldNotes_.end()) {
        heldNotes_.push_back(note);
        std::sort(heldNotes_.begin(), heldNotes_.end());
    }
    velocity_ = velocity;
    rebuildArpNotes();
}

void Arpeggiator::noteOff(int note) {
    heldNotes_.erase(std::remove(heldNotes_.begin(), heldNotes_.end(), note),
                     heldNotes_.end());
    rebuildArpNotes();
}

void Arpeggiator::rebuildArpNotes() {
    arpNotes_.clear();
    int octs = octaves_.load();
    for (int oct = 0; oct < octs; ++oct)
        for (int n : heldNotes_) {
            int shifted = n + oct * 12;
            if (shifted <= 127) arpNotes_.push_back(shifted);
        }

    // Zabezpiecz stepIndex_
    if (!arpNotes_.empty())
        stepIndex_ = stepIndex_ % (int)arpNotes_.size();
    else
        stepIndex_ = 0;
}

// ── Obliczenie długości kroku w próbkach ──────────────────────────────────────

double Arpeggiator::samplesPerStep() const {
    float bpm  = bpm_.load();
    int   div  = division_.load();
    // Długość ćwierćnuty w próbkach × (4/division)
    return sampleRate_ * (60.0 / bpm) * (4.0 / div);
}

// ── Przejście do następnego kroku ─────────────────────────────────────────────

void Arpeggiator::advanceStep() {
    if (arpNotes_.empty()) { stepIndex_ = 0; return; }
    int n = (int)arpNotes_.size();

    switch (getPattern()) {
        case Pattern::Up:
            stepIndex_ = (stepIndex_ + 1) % n;
            break;

        case Pattern::Down:
            stepIndex_ = (stepIndex_ - 1 + n) % n;
            break;

        case Pattern::UpDown:
            if (n == 1) { stepIndex_ = 0; break; }
            if (ascending_) {
                stepIndex_++;
                if (stepIndex_ >= n) { stepIndex_ = n - 2; ascending_ = false; }
            } else {
                stepIndex_--;
                if (stepIndex_ < 0) { stepIndex_ = 1; ascending_ = true; }
            }
            stepIndex_ = std::max(0, std::min(n - 1, stepIndex_));
            break;

        case Pattern::Random: {
            std::uniform_int_distribution<int> dist(0, n - 1);
            stepIndex_ = dist(rng_);
            break;
        }
    }
}

// ── Główna funkcja przetwarzania (wywoływana z processBlock) ──────────────────

std::vector<Arpeggiator::Event> Arpeggiator::process(int numSamples) {
    std::vector<Event> events;

    // Reset wymagany (np. arp wyłączony)
    if (needReset_.exchange(false)) {
        if (currentNote_ >= 0 && gateOpen_) {
            events.push_back({ false, currentNote_, 0.0f, 0 });
            gateOpen_ = false;
        }
        currentNote_ = -1;
        posInStep_   = 0;
        stepIndex_   = 0;
        ascending_   = true;
    }

    if (!enabled_.load()) return events;

    // Brak trzymanych nut — wycisz i czekaj
    if (arpNotes_.empty()) {
        if (currentNote_ >= 0 && gateOpen_) {
            events.push_back({ false, currentNote_, 0.0f, 0 });
            currentNote_ = -1;
            gateOpen_    = false;
        }
        posInStep_ = 0;  // reset fazy — zacznie od razu gdy przybędzie nuta
        return events;
    }

    int spStep  = std::max(1, static_cast<int>(samplesPerStep()));
    int gateLen = std::max(1, static_cast<int>(spStep * kGateRatio));

    for (int i = 0; i < numSamples; ++i) {
        // ── Początek kroku: NoteOn ─────────────────────────────────────────
        if (posInStep_ == 0) {
            // Zamknij poprzednią nutę jeśli jeszcze otwarta
            if (currentNote_ >= 0 && gateOpen_) {
                events.push_back({ false, currentNote_, 0.0f, i });
                gateOpen_ = false;
            }
            int note = arpNotes_[stepIndex_ % (int)arpNotes_.size()];
            events.push_back({ true, note, velocity_, i });
            currentNote_ = note;
            gateOpen_    = true;
        }
        // ── Zamknięcie gate ────────────────────────────────────────────────
        else if (posInStep_ == gateLen && gateOpen_) {
            events.push_back({ false, currentNote_, 0.0f, i });
            gateOpen_ = false;
        }

        if (++posInStep_ >= spStep) {
            posInStep_ = 0;
            advanceStep();
        }
    }

    return events;
}

} // namespace synth
