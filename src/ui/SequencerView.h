#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "engine/Sequencer.h"
#include <array>

// ── SequencerView — panel 16-krokowego sekwencera melodycznego ────────────────
// Górny pasek: [SEQ ON] [BPM-] [BPM label] [BPM+] [1/8] [1/16] [8 kroków] [16 kroków]
// Dolna siatka: 16 padów kroków — klik = toggle, scroll = zmiana nuty.
// Aktualnie grany krok świeci na pomarańczowo.
class SequencerView : public juce::Component,
                      private juce::Timer {
public:
    explicit SequencerView(synth::Sequencer& seq);
    ~SequencerView() override;

    // Ładuje stan sekwencera z presetu (wołać z UI thread)
    void applyState(float bpm, int div, int numSteps,
                    const std::vector<std::pair<int,bool>>& steps);

    void paint      (juce::Graphics& g)                              override;
    void resized    ()                                               override;
    void mouseDown  (const juce::MouseEvent& e)                      override;
    void mouseWheelMove(const juce::MouseEvent& e,
                        const juce::MouseWheelDetails& d)            override;

private:
    void timerCallback() override;

    int  stepAtPosition(juce::Point<int> p) const;
    static juce::String noteName(int midi);

    synth::Sequencer& seq_;

    std::array<juce::Rectangle<int>, synth::Sequencer::kMaxSteps> stepRects_;
    int lastStep_ = -1;

    // Kontrolki paska nagłówka
    juce::TextButton seqOnBtn_   { "SEQ" };
    juce::TextButton bpmDnBtn_   { "-" };
    juce::TextButton bpmUpBtn_   { "+" };
    juce::Label      bpmLabel_;
    juce::TextButton div8Btn_    { "1/8"  };
    juce::TextButton div16Btn_   { "1/16" };
    juce::TextButton steps8Btn_  { "8"    };
    juce::TextButton steps16Btn_ { "16"   };

    float seqBPM_   = 120.0f;
    int   seqDiv_   = 16;
    int   seqSteps_ = 8;

    void updateBpmLabel();
    void selectDiv  (int div);
    void selectSteps(int n);

    static constexpr int kHeaderH = 36;
};
