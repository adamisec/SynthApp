#include "KeyboardView.h"

// Czy dany semiton w oktawie jest czarnym klawiszem?
static constexpr bool kIsBlack[12] = {
    false, true, false, true, false,
    false, true, false, true, false, true, false
};

// Semiton dla n-tego bialego klawisza w oktawie (C D E F G A B)
static constexpr int kWhiteToSem[7] = { 0, 2, 4, 5, 7, 9, 11 };

// Indeks bialego klawisza po lewej stronie czarnego (-1 = nie czarny)
static constexpr int kBlackLeftWhite[12] = { -1, 0, -1, 1, -1, -1, 3, -1, 4, -1, 5, -1 };

bool KeyboardView::isBlackKey(int note) const {
    return kIsBlack[(note - kStartNote) % 12];
}

juce::Rectangle<float> KeyboardView::whiteKeyBounds(int whiteIdx) const {
    float wkw = (float)getWidth() / kNumWhite;
    return { whiteIdx * wkw, 0.0f, wkw - 0.5f, (float)getHeight() };
}

juce::Rectangle<float> KeyboardView::blackKeyBounds(int note) const {
    float wkw = (float)getWidth() / kNumWhite;
    float bkw = wkw * 0.62f;
    float bkh = (float)getHeight() * 0.62f;

    int offset = note - kStartNote;
    int octave = offset / 12;
    int s      = offset % 12;

    float x = (octave * 7 + kBlackLeftWhite[s] + 1) * wkw - bkw * 0.5f;
    return { x, 0.0f, bkw, bkh };
}

int KeyboardView::noteAtPosition(juce::Point<int> pos) const {
    float wkw = (float)getWidth() / kNumWhite;
    float bkh = (float)getHeight() * 0.62f;

    if ((float)pos.y < bkh) {
        constexpr int kBlackSems[] = { 1, 3, 6, 8, 10 };
        for (int oct = 0; oct < kNumOctaves; ++oct) {
            for (int s : kBlackSems) {
                int note = kStartNote + oct * 12 + s;
                if (blackKeyBounds(note).contains(pos.toFloat()))
                    return note;
            }
        }
    }

    int whiteIdx = (int)((float)pos.x / wkw);
    if (whiteIdx < 0 || whiteIdx >= kNumWhite) return -1;
    int octave = whiteIdx / 7;
    int inOct  = whiteIdx % 7;
    return kStartNote + octave * 12 + kWhiteToSem[inOct];
}

void KeyboardView::paint(juce::Graphics& g) {
    // Tlo klawiatury — ciemna listwa
    g.fillAll(juce::Colour(0xFF111111));

    // Gorny pasek — Nord czerwien (metaliczna listwa nad klawiatura)
    g.setColour(juce::Colour(0xFF881500));
    g.fillRect(0, 0, getWidth(), 5);
    g.setColour(juce::Colour(0xFF080808));
    g.drawLine(0.0f, 5.0f, (float)getWidth(), 5.0f, 1.0f);

    // Biale klawisze — kolor kosci sloniowej z gradientem
    for (int i = 0; i < kNumWhite; ++i) {
        int octave   = i / 7;
        int note     = kStartNote + octave * 12 + kWhiteToSem[i % 7];
        bool pressed = (note == pressedNote_);

        auto b = whiteKeyBounds(i);

        juce::ColourGradient kg(
            pressed ? juce::Colour(0xFFCCCBC4) : juce::Colour(0xFFF5F1E8), b.getX(), b.getY(),
            pressed ? juce::Colour(0xFFB8B4AC) : juce::Colour(0xFFE2DDD4), b.getX(), b.getBottom(), false);
        g.setGradientFill(kg);
        g.fillRect(b);

        // Krawedz klawisza
        g.setColour(juce::Colour(0xFF777777));
        g.drawRect(b, 0.5f);
    }

    // Czarne klawisze z gradientem
    constexpr int kBlackSems[] = { 1, 3, 6, 8, 10 };
    for (int oct = 0; oct < kNumOctaves; ++oct) {
        for (int s : kBlackSems) {
            int  note    = kStartNote + oct * 12 + s;
            bool pressed = (note == pressedNote_);
            auto b       = blackKeyBounds(note);

            juce::ColourGradient kg(
                pressed ? juce::Colour(0xFF555550) : juce::Colour(0xFF282828), b.getX(), b.getY(),
                pressed ? juce::Colour(0xFF333330) : juce::Colour(0xFF0A0A0A), b.getX(), b.getBottom(), false);
            g.setGradientFill(kg);
            g.fillRect(b);

            // Subtelna krawedz
            g.setColour(juce::Colour(0xFF555555));
            g.drawRect(b, 0.5f);

            // Polysk na gorze czarnego klawisza
            g.setColour(juce::Colour(0x20FFFFFF));
            g.fillRect(b.getX() + 1.0f, b.getY() + 1.0f, b.getWidth() - 2.0f, 3.0f);
        }
    }
}

void KeyboardView::mouseDown(const juce::MouseEvent& e) {
    int note = noteAtPosition(e.getPosition());
    if (note >= kStartNote && note < kStartNote + kNumOctaves * 12) {
        pressedNote_ = note;
        if (onNoteOn) onNoteOn(note, 0.8f);
        repaint();
    }
}

void KeyboardView::mouseDrag(const juce::MouseEvent& e) {
    int note = noteAtPosition(e.getPosition());
    if (note == pressedNote_) return;

    if (pressedNote_ >= 0 && onNoteOff) onNoteOff(pressedNote_);
    pressedNote_ = -1;

    if (note >= kStartNote && note < kStartNote + kNumOctaves * 12) {
        pressedNote_ = note;
        if (onNoteOn) onNoteOn(note, 0.8f);
    }
    repaint();
}

void KeyboardView::mouseUp(const juce::MouseEvent& /*e*/) {
    if (pressedNote_ >= 0 && onNoteOff) onNoteOff(pressedNote_);
    pressedNote_ = -1;
    repaint();
}
