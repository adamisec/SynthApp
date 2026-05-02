#include "Sequencer.h"

namespace synth {

Sequencer::Sequencer() {
    // Domyślna gama C-dur przez 2 oktawy (8 kroków aktywnych)
    static const int kDefaultNotes[kMaxSteps] = {
        60, 62, 64, 65, 67, 69, 71, 72,
        74, 76, 77, 79, 81, 83, 84, 86
    };
    for (int i = 0; i < kMaxSteps; ++i) {
        stepNote_[i].store(kDefaultNotes[i]);
        stepActive_[i].store(i < 8);
    }
}

void Sequencer::prepare(double sampleRate) {
    sampleRate_  = sampleRate;
    posInStep_   = 0;
    step_        = -1;
    currentNote_ = -1;
    gateOpen_    = false;
    currentStep_.store(0);
}

double Sequencer::samplesPerStep() const {
    float bpm = bpm_.load();
    int   div = division_.load();
    return sampleRate_ * (60.0 / bpm) * (4.0 / div);
}

std::vector<Sequencer::Event> Sequencer::process(int numSamples) {
    std::vector<Event> events;

    // Reset wymagany (np. wyłączony sekwencer)
    if (needReset_.exchange(false)) {
        if (currentNote_ >= 0 && gateOpen_) {
            events.push_back({ false, currentNote_, 0.0f, 0 });
            gateOpen_ = false;
        }
        currentNote_ = -1;
        posInStep_   = 0;
        step_        = -1;
        currentStep_.store(0);
    }

    if (!enabled_.load()) return events;

    int spStep  = std::max(1, static_cast<int>(samplesPerStep()));
    int gateLen = std::max(1, static_cast<int>(spStep * kGateRatio));
    int nSteps  = numSteps_.load();

    for (int i = 0; i < numSamples; ++i) {
        if (posInStep_ == 0) {
            // Przejście do następnego kroku
            step_ = (step_ + 1) % nSteps;
            currentStep_.store(step_);

            // Zamknij poprzednią nutę
            if (currentNote_ >= 0 && gateOpen_) {
                events.push_back({ false, currentNote_, 0.0f, i });
                gateOpen_ = false;
            }

            // NoteOn jeśli krok aktywny
            if (stepActive_[step_].load()) {
                int note = std::max(0, std::min(127, stepNote_[step_].load()));
                events.push_back({ true, note, 0.8f, i });
                currentNote_ = note;
                gateOpen_    = true;
            } else {
                currentNote_ = -1;
            }
        } else if (posInStep_ == gateLen && gateOpen_) {
            events.push_back({ false, currentNote_, 0.0f, i });
            gateOpen_ = false;
        }

        if (++posInStep_ >= spStep)
            posInStep_ = 0;
    }

    return events;
}

} // namespace synth
