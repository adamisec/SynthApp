#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

// ── KeyboardView — wirtualna klawiatura MIDI ──────────────────────────────────
// 3 oktawy (C3–B5, MIDI 48–83). Kliknięcie/przeciągnięcie = Note On/Off.
class KeyboardView : public juce::Component {
public:
    std::function<void(int note, float velocity)> onNoteOn;
    std::function<void(int note)>                 onNoteOff;

    void paint   (juce::Graphics& g)           override;
    void mouseDown(const juce::MouseEvent& e)  override;
    void mouseDrag(const juce::MouseEvent& e)  override;
    void mouseUp  (const juce::MouseEvent& e)  override;

private:
    static constexpr int kStartNote  = 48; // C3
    static constexpr int kNumOctaves = 3;
    static constexpr int kNumWhite   = kNumOctaves * 7;  // 21

    int pressedNote_ = -1;

    int  noteAtPosition(juce::Point<int> pos) const;
    bool isBlackKey    (int note)             const;

    juce::Rectangle<float> whiteKeyBounds(int whiteIdx) const;
    juce::Rectangle<float> blackKeyBounds(int note)     const;
};
