#include "SequencerView.h"

// ── Pomocnik stylu przycisków (duplikujemy wzorzec z PluginEditor) ─────────────
namespace {

void styleSeqBtn(juce::TextButton& b,
                 juce::Colour bg   = juce::Colour(0xFF1E1E1E),
                 juce::Colour text = juce::Colour(0xFFCCCCCC))
{
    b.setColour(juce::TextButton::buttonColourId,   bg);
    b.setColour(juce::TextButton::buttonOnColourId, bg.brighter(0.2f));
    b.setColour(juce::TextButton::textColourOffId,  text);
    b.setColour(juce::TextButton::textColourOnId,   text.brighter(0.3f));
}

} // namespace

// ── Konstruktor ────────────────────────────────────────────────────────────────

SequencerView::SequencerView(synth::Sequencer& seq)
    : seq_(seq)
{
    // SEQ ON/OFF
    styleSeqBtn(seqOnBtn_, juce::Colour(0xFF1A2A18), juce::Colour(0xFF44CC66));
    seqOnBtn_.setClickingTogglesState(true);
    seqOnBtn_.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF1E4A28));
    seqOnBtn_.onClick = [this] {
        seq_.setEnabled(seqOnBtn_.getToggleState());
    };
    seqOnBtn_.setMouseClickGrabsKeyboardFocus(false);
    seqOnBtn_.setWantsKeyboardFocus(false);
    addAndMakeVisible(seqOnBtn_);

    // BPM
    bpmLabel_.setJustificationType(juce::Justification::centred);
    bpmLabel_.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    bpmLabel_.setColour(juce::Label::textColourId, juce::Colour(0xFFFF9933));
    addAndMakeVisible(bpmLabel_);
    updateBpmLabel();

    styleSeqBtn(bpmDnBtn_, juce::Colour(0xFF1A1A1A), juce::Colour(0xFFCCCCCC));
    bpmDnBtn_.onClick = [this] {
        seqBPM_ = std::max(40.0f, seqBPM_ - 5.0f);
        seq_.setBPM(seqBPM_);
        updateBpmLabel();
    };
    bpmDnBtn_.setMouseClickGrabsKeyboardFocus(false);
    bpmDnBtn_.setWantsKeyboardFocus(false);
    addAndMakeVisible(bpmDnBtn_);

    styleSeqBtn(bpmUpBtn_, juce::Colour(0xFF1A1A1A), juce::Colour(0xFFCCCCCC));
    bpmUpBtn_.onClick = [this] {
        seqBPM_ = std::min(280.0f, seqBPM_ + 5.0f);
        seq_.setBPM(seqBPM_);
        updateBpmLabel();
    };
    bpmUpBtn_.setMouseClickGrabsKeyboardFocus(false);
    bpmUpBtn_.setWantsKeyboardFocus(false);
    addAndMakeVisible(bpmUpBtn_);

    // Podział rytmiczny
    styleSeqBtn(div8Btn_,  juce::Colour(0xFF1A1A1A), juce::Colour(0xFF888888));
    div8Btn_.onClick  = [this] { selectDiv(8);  };
    div8Btn_.setMouseClickGrabsKeyboardFocus(false);
    div8Btn_.setWantsKeyboardFocus(false);
    addAndMakeVisible(div8Btn_);

    styleSeqBtn(div16Btn_, juce::Colour(0xFF1A2A3A), juce::Colour(0xFF55AAFF));
    div16Btn_.onClick = [this] { selectDiv(16); };
    div16Btn_.setMouseClickGrabsKeyboardFocus(false);
    div16Btn_.setWantsKeyboardFocus(false);
    addAndMakeVisible(div16Btn_);

    // Liczba kroków
    styleSeqBtn(steps8Btn_,  juce::Colour(0xFF1A2A3A), juce::Colour(0xFF55AAFF));
    steps8Btn_.onClick  = [this] { selectSteps(8);  };
    steps8Btn_.setMouseClickGrabsKeyboardFocus(false);
    steps8Btn_.setWantsKeyboardFocus(false);
    addAndMakeVisible(steps8Btn_);

    styleSeqBtn(steps16Btn_, juce::Colour(0xFF1A1A1A), juce::Colour(0xFF888888));
    steps16Btn_.onClick = [this] { selectSteps(16); };
    steps16Btn_.setMouseClickGrabsKeyboardFocus(false);
    steps16Btn_.setWantsKeyboardFocus(false);
    addAndMakeVisible(steps16Btn_);

    // Ustaw wartości domyślne w silniku
    seq_.setBPM(seqBPM_);
    selectDiv(seqDiv_);
    selectSteps(seqSteps_);

    startTimerHz(20);  // odświeżanie pozycji kroku
}

SequencerView::~SequencerView() {
    stopTimer();
}

// ── Ładowanie stanu z presetu ─────────────────────────────────────────────────

void SequencerView::applyState(float bpm, int div, int numSteps,
                                const std::vector<std::pair<int,bool>>& steps) {
    seqBPM_ = bpm;
    seq_.setBPM(bpm);
    updateBpmLabel();
    selectDiv(numSteps > 0 ? div : seqDiv_);
    selectSteps(numSteps > 0 ? numSteps : seqSteps_);

    for (int i = 0; i < synth::Sequencer::kMaxSteps; ++i) {
        if (i < (int)steps.size()) {
            seq_.setStepNote  (i, steps[i].first);
            seq_.setStepActive(i, steps[i].second);
        } else {
            seq_.setStepActive(i, false);
        }
    }
    repaint();
}

// ── Timer — animacja aktualnego kroku ─────────────────────────────────────────

void SequencerView::timerCallback() {
    int cur = seq_.isEnabled() ? seq_.currentStep() : -1;
    if (cur != lastStep_) {
        lastStep_ = cur;
        repaint();
    }
}

// ── Pomocniki ─────────────────────────────────────────────────────────────────

juce::String SequencerView::noteName(int midi) {
    static const char* names[] = {
        "C","C#","D","D#","E","F","F#","G","G#","A","A#","B"
    };
    if (midi < 0 || midi > 127) return "-";
    return juce::String(names[midi % 12]) + juce::String(midi / 12 - 1);
}

void SequencerView::updateBpmLabel() {
    bpmLabel_.setText("BPM: " + juce::String((int)seqBPM_),
                      juce::dontSendNotification);
}

void SequencerView::selectDiv(int div) {
    seqDiv_ = div;
    seq_.setDivision(div);

    const juce::Colour actBg   { 0xFF1A2A3A };
    const juce::Colour actTxt  { 0xFF55AAFF };
    const juce::Colour idleBg  { 0xFF1A1A1A };
    const juce::Colour idleTxt { 0xFF888888 };

    auto hi = [&](juce::TextButton& b, bool on) {
        b.setColour(juce::TextButton::buttonColourId,  on ? actBg  : idleBg);
        b.setColour(juce::TextButton::textColourOffId, on ? actTxt : idleTxt);
    };
    hi(div8Btn_,  div == 8);
    hi(div16Btn_, div == 16);
    repaint();
}

void SequencerView::selectSteps(int n) {
    seqSteps_ = n;
    seq_.setNumSteps(n);

    const juce::Colour actBg   { 0xFF2A1A2A };
    const juce::Colour actTxt  { 0xFFCC77FF };
    const juce::Colour idleBg  { 0xFF1A1A1A };
    const juce::Colour idleTxt { 0xFF888888 };

    auto hi = [&](juce::TextButton& b, bool on) {
        b.setColour(juce::TextButton::buttonColourId,  on ? actBg  : idleBg);
        b.setColour(juce::TextButton::textColourOffId, on ? actTxt : idleTxt);
    };
    hi(steps8Btn_,  n == 8);
    hi(steps16Btn_, n == 16);
    repaint();
}

// ── Rozmieszczenie ─────────────────────────────────────────────────────────────

void SequencerView::resized() {
    int w = getWidth();
    int h = getHeight();

    // Pasek nagłówka: y=4, height=28
    int by = 4, bh = kHeaderH - 8;
    int x  = 8;

    seqOnBtn_.setBounds(x,     by, 52, bh); x += 56;
    bpmLabel_.setBounds(x,     by, 72, bh); x += 74;
    bpmDnBtn_.setBounds(x,     by, 22, bh); x += 24;
    bpmUpBtn_.setBounds(x,     by, 22, bh); x += 30;
    div8Btn_ .setBounds(x,     by, 38, bh); x += 40;
    div16Btn_.setBounds(x,     by, 38, bh); x += 46;
    steps8Btn_ .setBounds(x,   by, 28, bh); x += 30;
    steps16Btn_.setBounds(x,   by, 32, bh);

    // Siatka kroków — pełna szerokość komponentu
    const int stepY  = kHeaderH + 4;
    const int stepH  = h - stepY - 4;
    const int nSteps = synth::Sequencer::kMaxSteps;
    const int gap    = 2;
    const int totalGap = gap * (nSteps - 1);
    int stepW = (w - 16 - totalGap) / nSteps;

    for (int i = 0; i < nSteps; ++i) {
        int sx = 8 + i * (stepW + gap);
        stepRects_[i] = juce::Rectangle<int>(sx, stepY, stepW, stepH);
    }
}

// ── Rysowanie ─────────────────────────────────────────────────────────────────

void SequencerView::paint(juce::Graphics& g) {
    int w = getWidth();
    int h = getHeight();

    // Tło
    g.setColour(juce::Colour(0xFF080808));
    g.fillRect(0, 0, w, h);

    // Górna linia separatora
    g.setColour(juce::Colour(0xFF1C1C1C));
    g.fillRect(0, 0, w, 1);

    // Etykiety sekcji nagłówka
    g.setFont(juce::FontOptions(8.0f).withStyle("Bold"));
    g.setColour(juce::Colour(0xFF444444));
    g.drawText("BPM",   82,  0, 120,  kHeaderH, juce::Justification::centred, false);
    g.drawText("RATE",  260, 0,  96,  kHeaderH, juce::Justification::centred, false);
    g.drawText("STEPS", 360, 0,  68,  kHeaderH, juce::Justification::centred, false);

    // Linia separatora między nagłówkiem a siatką
    g.setColour(juce::Colour(0xFF151515));
    g.fillRect(0, kHeaderH, w, 2);

    // ── Pady kroków ─────────────────────────────────────────────────────────
    int nActive = seq_.getNumSteps();

    for (int i = 0; i < synth::Sequencer::kMaxSteps; ++i) {
        auto r = stepRects_[i].toFloat();
        bool inRange = (i < nActive);
        bool active  = inRange && seq_.getStepActive(i);
        bool playing = seq_.isEnabled() && (i == lastStep_) && inRange;

        // Tło padu
        juce::Colour bg;
        if (playing)
            bg = juce::Colour(0xFFFF8800);  // aktualnie grany — jaskrawy pomarańcz
        else if (active)
            bg = juce::Colour(0xFF7A4400);  // aktywny krok — ciemny amber
        else if (inRange)
            bg = juce::Colour(0xFF181818);  // w zakresie, nieaktywny
        else
            bg = juce::Colour(0xFF0E0E0E);  // poza zakresem

        // Cień
        g.setColour(juce::Colour(0x55000000));
        g.fillRoundedRectangle(r.translated(0.0f, 1.5f), 4.0f);

        // Pad
        g.setColour(bg);
        g.fillRoundedRectangle(r, 4.0f);

        // Górny blask
        g.setColour(juce::Colour(0x10FFFFFF));
        g.fillRoundedRectangle(r.withBottom(r.getCentreY()), 4.0f);

        // Krawędź
        juce::Colour border = playing ? juce::Colour(0xFFFFCC00)
                            : active  ? juce::Colour(0xFFCC6600).withAlpha(0.7f)
                                      : juce::Colour(0xFF2A2A2A);
        g.setColour(border);
        g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, 1.0f);

        if (!inRange) continue;

        // Numer kroku (mały, u góry)
        g.setFont(juce::FontOptions(8.0f));
        g.setColour(playing ? juce::Colour(0xFFFFEE88)
                  : active  ? juce::Colour(0xFF996633)
                            : juce::Colour(0xFF333333));
        g.drawText(juce::String(i + 1), r.toNearestInt().withHeight(14),
                   juce::Justification::centred, false);

        // Nazwa nuty (środek)
        if (active || playing) {
            int note = seq_.getStepNote(i);
            g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
            g.setColour(playing ? juce::Colours::white : juce::Colour(0xFFFFCC88));
            auto noteR = r.toNearestInt();
            noteR.setY(noteR.getY() + 14);
            noteR.setHeight(noteR.getHeight() - 14);
            g.drawText(noteName(note), noteR, juce::Justification::centred, false);
        }
    }
}

// ── Interakcja myszy ──────────────────────────────────────────────────────────

int SequencerView::stepAtPosition(juce::Point<int> p) const {
    for (int i = 0; i < synth::Sequencer::kMaxSteps; ++i)
        if (stepRects_[i].contains(p))
            return i;
    return -1;
}

void SequencerView::mouseDown(const juce::MouseEvent& e) {
    int i = stepAtPosition(e.getPosition());
    if (i < 0 || i >= seq_.getNumSteps()) return;

    // Toggle aktywności kroku
    bool nowActive = !seq_.getStepActive(i);
    seq_.setStepActive(i, nowActive);
    repaint();
}

void SequencerView::mouseWheelMove(const juce::MouseEvent& e,
                                   const juce::MouseWheelDetails& d) {
    int i = stepAtPosition(e.getPosition());
    if (i < 0 || i >= seq_.getNumSteps()) return;

    // Scroll w górę = wyższa nuta, w dół = niższa
    int delta = (d.deltaY > 0.0f) ? 1 : -1;
    if (e.mods.isCtrlDown()) delta *= 12;  // Ctrl = skok oktawy

    int newNote = std::max(0, std::min(127, seq_.getStepNote(i) + delta));
    seq_.setStepNote(i, newNote);
    repaint();
}
