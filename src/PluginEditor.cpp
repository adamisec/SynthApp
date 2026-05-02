#include "PluginEditor.h"
#include "engine/AudioEngine.h"
#include <algorithm>

// ── Nowoczesny LookAndFeel przycisków ─────────────────────────────────────────
class ModernLF : public juce::LookAndFeel_V4 {
public:
    void drawButtonBackground(juce::Graphics& g, juce::Button& btn,
                              const juce::Colour& bgColour,
                              bool isMouseOver, bool isButtonDown) override
    {
        auto b = btn.getLocalBounds().toFloat().reduced(0.5f);
        const float r = 5.0f;

        auto col = isButtonDown ? bgColour.darker(0.12f)
                                : (isMouseOver ? bgColour.brighter(0.12f) : bgColour);
        // Cień
        if (!isButtonDown) {
            g.setColour(juce::Colour(0x44000000));
            g.fillRoundedRectangle(b.translated(0.0f, 1.5f), r);
        }
        g.setColour(col);
        g.fillRoundedRectangle(b, r);

        // Połysk na górnej połowie
        g.setColour(juce::Colour(0x0EFFFFFF));
        g.fillRoundedRectangle(b.withBottom(b.getCentreY()), r);

        // Krawędź
        g.setColour(col.brighter(0.4f).withAlpha(0.55f));
        g.drawRoundedRectangle(b, r, 1.0f);
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& btn,
                        bool /*isMouseOver*/, bool isButtonDown) override
    {
        auto col = btn.findColour(juce::TextButton::textColourOffId);
        if (isButtonDown) col = col.darker(0.1f);
        g.setColour(col);
        g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
        g.drawText(btn.getButtonText(), btn.getLocalBounds(),
                   juce::Justification::centred, false);
    }

    void drawComboBox(juce::Graphics& g, int width, int height,
                      bool /*isDown*/, int bx, int by, int bw, int bh,
                      juce::ComboBox& box) override
    {
        auto b = juce::Rectangle<float>(0, 0, (float)width, (float)height).reduced(0.5f);
        g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle(b, 5.0f);
        g.setColour(box.findColour(juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle(b, 5.0f, 1.0f);

        float ax = (float)(bx + bw * 0.5f);
        float ay = (float)(by + bh * 0.5f);
        g.setColour(box.findColour(juce::ComboBox::arrowColourId));
        juce::Path arrow;
        arrow.addTriangle(ax - 4.5f, ay - 2.0f, ax + 4.5f, ay - 2.0f, ax, ay + 3.5f);
        g.fillPath(arrow);
    }
};

static ModernLF sModernLF;

// ── Pomocnik stylu przycisków ─────────────────────────────────────────────────
static void styleBtn(juce::TextButton& b,
                     juce::Colour bg   = juce::Colour(0xFF1E1E1E),
                     juce::Colour text = juce::Colour(0xFFCCCCCC))
{
    b.setColour(juce::TextButton::buttonColourId,   bg);
    b.setColour(juce::TextButton::buttonOnColourId, bg.brighter(0.18f));
    b.setColour(juce::TextButton::textColourOffId,  text);
    b.setColour(juce::TextButton::textColourOnId,   text);
    b.setLookAndFeel(&sModernLF);
}

// ── Mapowanie klawiszy → MIDI (baza C4 = 60, bez przesuniecia oktawy) ────────
//
//  Rząd cyfr:  2  3     5  6  7         C#5 D#5  F#5 G#5 A#5
//  Rząd QWERTY: q  w  e  r  t  y  u  i   C5  D5  E5  F5  G5  A5  B5  C6
//  Rząd ASDF:     s  d     g  h  j       C#4 D#4  F#4 G#4 A#4
//  Rząd ZXCV:  z  x  c  v  b  n  m      C4  D4  E4  F4  G4  A4  B4
//
int PluginEditor::keyCodeToMidi(int kc) {
    static const std::unordered_map<int,int> map = {
        // Dolna oktawa — białe (zxcvbnm)
        {'Z', 60}, {'X', 62}, {'C', 64}, {'V', 65},
        {'B', 67}, {'N', 69}, {'M', 71},
        // Dolna oktawa — czarne (sdfghj)
        {'S', 61}, {'D', 63}, {'G', 66}, {'H', 68}, {'J', 70},
        // Górna oktawa — białe (qwertyui)
        {'Q', 72}, {'W', 74}, {'E', 76}, {'R', 77},
        {'T', 79}, {'Y', 81}, {'U', 83}, {'I', 84},
        // Górna oktawa — czarne (cyfry 2 3 5 6 7)
        {'2', 73}, {'3', 75}, {'5', 78}, {'6', 80}, {'7', 82},
    };
    auto it = map.find(kc);
    return (it != map.end()) ? it->second : -1;
}

// ── Konstruktor ───────────────────────────────────────────────────────────────

PluginEditor::PluginEditor(synth::AudioEngine& engine)
    : AudioProcessorEditor(&engine),
      engine_(engine),
      seqView_(engine.sequencer())
{
    setSize(1020, 820);
    setWantsKeyboardFocus(true);

    setLookAndFeel(&sModernLF);

    // ── Tytuł ─────────────────────────────────────────────────────────────────
    titleLabel_.setText("SynthApp G2", juce::dontSendNotification);
    titleLabel_.setFont(juce::FontOptions(16.0f).withStyle("Bold"));
    titleLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel_);

    // ── ComboBox presetów ──────────────────────────────────────────────────────
    presetBox_.setTextWhenNothingSelected("-- Preset --");
    presetBox_.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF1A1A1A));
    presetBox_.setColour(juce::ComboBox::textColourId,       juce::Colour(0xFFDDDDDD));
    presetBox_.setColour(juce::ComboBox::outlineColourId,    juce::Colour(0xFF444444));
    presetBox_.setColour(juce::ComboBox::arrowColourId,      juce::Colour(0xFF888888));
    presetBox_.setLookAndFeel(&sModernLF);
    addAndMakeVisible(presetBox_);

    auto names = presets_.getNames();
    for (int i = 0; i < names.size(); ++i)
        presetBox_.addItem(names[i], i + 1);
    presetBox_.setSelectedId(1, juce::dontSendNotification);
    presetBox_.onChange = [this] {
        int id = presetBox_.getSelectedId();
        if (id > 0) loadPreset(id - 1);
    };

    // ── Przyciski nawigacji presetów ──────────────────────────────────────────
    styleBtn(prevBtn_, juce::Colour(0xFF1A1A1A), juce::Colour(0xFFAAAAAA));
    prevBtn_.onClick = [this] {
        int n = currentPreset_ - 1;
        if (n < 0) n = presets_.getCount() - 1;
        presetBox_.setSelectedId(n + 1);
    };
    addAndMakeVisible(prevBtn_);

    styleBtn(nextBtn_, juce::Colour(0xFF1A1A1A), juce::Colour(0xFFAAAAAA));
    nextBtn_.onClick = [this] {
        int n = (currentPreset_ + 1) % presets_.getCount();
        presetBox_.setSelectedId(n + 1);
    };
    addAndMakeVisible(nextBtn_);

    styleBtn(saveBtn_, juce::Colour(0xFF162A1E), juce::Colour(0xFF55EE88));
    saveBtn_.onClick = [this] { saveCurrentPreset(); };
    addAndMakeVisible(saveBtn_);

    styleBtn(deleteBtn_, juce::Colour(0xFF2A1414), juce::Colour(0xFFFF5544));
    deleteBtn_.onClick = [this] {
        auto* p = presets_.getPreset(currentPreset_);
        if (!p || p->isFactory) {
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
                "Nie mozna usunac", "Presety fabryczne sa chronione.");
            return;
        }
        presets_.deletePreset(p->name);
        presetBox_.clear(juce::dontSendNotification);
        auto n2 = presets_.getNames();
        for (int i = 0; i < n2.size(); ++i) presetBox_.addItem(n2[i], i + 1);
        currentPreset_ = 0;
        presetBox_.setSelectedId(1);
    };
    addAndMakeVisible(deleteBtn_);

    // ── Oktawa klawiatury PC ───────────────────────────────────────────────────
    styleBtn(octDownBtn_, juce::Colour(0xFF1A1A1A), juce::Colour(0xFFAAAAAA));
    octDownBtn_.onClick = [this] {
        octaveShift_ = std::max(octaveShift_ - 1, -3);
        updateOctaveLabel();
    };
    addAndMakeVisible(octDownBtn_);

    octLabel_.setJustificationType(juce::Justification::centred);
    octLabel_.setFont(juce::FontOptions(10.0f));
    octLabel_.setColour(juce::Label::textColourId, juce::Colour(0xFF999999));
    addAndMakeVisible(octLabel_);
    updateOctaveLabel();

    styleBtn(octUpBtn_, juce::Colour(0xFF1A1A1A), juce::Colour(0xFFAAAAAA));
    octUpBtn_.onClick = [this] {
        octaveShift_ = std::min(octaveShift_ + 1, 3);
        updateOctaveLabel();
    };
    addAndMakeVisible(octUpBtn_);

    // ── AUDIO ─────────────────────────────────────────────────────────────────
    styleBtn(audioBtn_, juce::Colour(0xFF14202E), juce::Colour(0xFF6699FF));
    audioBtn_.onClick = [this] { openAudioSettings(); };
    addAndMakeVisible(audioBtn_);

    // ── Perf ──────────────────────────────────────────────────────────────────
    perfLabel_.setJustificationType(juce::Justification::centredRight);
    perfLabel_.setColour(juce::Label::textColourId, juce::Colour(0xFF666666));
    perfLabel_.setFont(juce::FontOptions(10.0f));
    addAndMakeVisible(perfLabel_);

    // ── Arpeggiator UI ────────────────────────────────────────────────────────

    // Przycisk ON/OFF
    styleBtn(arpOnBtn_, juce::Colour(0xFF162A1A), juce::Colour(0xFF44DD77));
    arpOnBtn_.setClickingTogglesState(true);
    arpOnBtn_.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF1E4A28));
    arpOnBtn_.onClick = [this] {
        bool on = arpOnBtn_.getToggleState();
        engine_.setArpEnabled(on);
    };
    addAndMakeVisible(arpOnBtn_);

    // Przyciski wzorca — helper lambda
    auto setupPatternBtn = [&](juce::TextButton& btn, synth::Arpeggiator::Pattern pat) {
        styleBtn(btn, juce::Colour(0xFF1A1A1A), juce::Colour(0xFFAAAAAA));
        btn.setClickingTogglesState(false);
        btn.onClick = [this, pat, &btn] { selectArpPattern(pat); };
        btn.setMouseClickGrabsKeyboardFocus(false);
        btn.setWantsKeyboardFocus(false);
        addAndMakeVisible(btn);
    };
    setupPatternBtn(arpUpBtn_,  synth::Arpeggiator::Pattern::Up);
    setupPatternBtn(arpDnBtn_,  synth::Arpeggiator::Pattern::Down);
    setupPatternBtn(arpUdBtn_,  synth::Arpeggiator::Pattern::UpDown);
    setupPatternBtn(arpRndBtn_, synth::Arpeggiator::Pattern::Random);

    // BPM
    bpmLabel_.setJustificationType(juce::Justification::centred);
    bpmLabel_.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    bpmLabel_.setColour(juce::Label::textColourId, juce::Colour(0xFFFF9933));
    addAndMakeVisible(bpmLabel_);
    updateBpmLabel();

    styleBtn(bpmDnBtn_, juce::Colour(0xFF1A1A1A), juce::Colour(0xFFCCCCCC));
    bpmDnBtn_.onClick = [this] {
        arpBPM_ = std::max(40.0f, arpBPM_ - 5.0f);
        engine_.setArpBPM(arpBPM_);
        updateBpmLabel();
    };
    addAndMakeVisible(bpmDnBtn_);

    styleBtn(bpmUpBtn_, juce::Colour(0xFF1A1A1A), juce::Colour(0xFFCCCCCC));
    bpmUpBtn_.onClick = [this] {
        arpBPM_ = std::min(280.0f, arpBPM_ + 5.0f);
        engine_.setArpBPM(arpBPM_);
        updateBpmLabel();
    };
    addAndMakeVisible(bpmUpBtn_);

    // Przyciski tempa rytmicznego
    auto setupRateBtn = [&](juce::TextButton& btn, int div) {
        styleBtn(btn, juce::Colour(0xFF1A1A1A), juce::Colour(0xFFAAAAAA));
        btn.onClick = [this, div] { selectArpRate(div); };
        btn.setMouseClickGrabsKeyboardFocus(false);
        btn.setWantsKeyboardFocus(false);
        addAndMakeVisible(btn);
    };
    setupRateBtn(rate4Btn_,  4);
    setupRateBtn(rate8Btn_,  8);
    setupRateBtn(rate16Btn_, 16);
    setupRateBtn(rate32Btn_, 32);

    // Przyciski oktaw
    auto setupOctBtn = [&](juce::TextButton& btn, int oct) {
        styleBtn(btn, juce::Colour(0xFF1A1A1A), juce::Colour(0xFFAAAAAA));
        btn.onClick = [this, oct] { selectArpOct(oct); };
        btn.setMouseClickGrabsKeyboardFocus(false);
        btn.setWantsKeyboardFocus(false);
        addAndMakeVisible(btn);
    };
    setupOctBtn(oct1Btn_, 1);
    setupOctBtn(oct2Btn_, 2);
    setupOctBtn(oct3Btn_, 3);

    // Ustaw zaznaczenie domyślne
    selectArpPattern(arpPattern_);
    selectArpRate   (arpRate_);
    selectArpOct    (arpOct_);

    // Wyłącz kradzież focusa przez kontrolki arpa
    for (auto* c : { (juce::Component*)&arpOnBtn_,
                     (juce::Component*)&arpUpBtn_,  (juce::Component*)&arpDnBtn_,
                     (juce::Component*)&arpUdBtn_,  (juce::Component*)&arpRndBtn_,
                     (juce::Component*)&bpmDnBtn_,  (juce::Component*)&bpmUpBtn_,
                     (juce::Component*)&rate4Btn_,  (juce::Component*)&rate8Btn_,
                     (juce::Component*)&rate16Btn_, (juce::Component*)&rate32Btn_,
                     (juce::Component*)&oct1Btn_,   (juce::Component*)&oct2Btn_,
                     (juce::Component*)&oct3Btn_ }) {
        c->setMouseClickGrabsKeyboardFocus(false);
        c->setWantsKeyboardFocus(false);
    }

    // ── Panel, sekwencer i klawiatura ─────────────────────────────────────────
    panel_.onParamChanged = [this](int modId, int paramIdx, float v) {
        engine_.setModuleParam(modId, paramIdx, v);
    };
    addAndMakeVisible(panel_);

    seqView_.setMouseClickGrabsKeyboardFocus(false);
    addAndMakeVisible(seqView_);

    keyboard_.onNoteOn  = [this](int note, float vel) { engine_.noteOn(note, vel); };
    keyboard_.onNoteOff = [this](int note)             { engine_.noteOff(note); };
    addAndMakeVisible(keyboard_);

    // Zapobiegaj kradzieży focusa przez kontrolki — PluginEditor zawsze
    // trzyma focus klawiatury, dzięki czemu keyPressed działa globalnie
    for (auto* c : { (juce::Component*)&presetBox_,
                     (juce::Component*)&prevBtn_,   (juce::Component*)&nextBtn_,
                     (juce::Component*)&saveBtn_,   (juce::Component*)&deleteBtn_,
                     (juce::Component*)&octDownBtn_, (juce::Component*)&octUpBtn_,
                     (juce::Component*)&audioBtn_ }) {
        c->setMouseClickGrabsKeyboardFocus(false);
        c->setWantsKeyboardFocus(false);
    }
    panel_   .setMouseClickGrabsKeyboardFocus(false);
    keyboard_.setMouseClickGrabsKeyboardFocus(false);

    loadPreset(0);
    startTimerHz(10);
    grabKeyboardFocus();
}

PluginEditor::~PluginEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
    presetBox_.setLookAndFeel(nullptr);
    for (auto& [kc, note] : heldKeys_)
        engine_.noteOff(note);
}

// ── Rysowanie ─────────────────────────────────────────────────────────────────

void PluginEditor::paint(juce::Graphics& g) {
    int w = getWidth();
    int h = getHeight();
    int panelH = h - kHeaderH - kSeqH - kArpH - kKeyboardH;
    int seqY   = kHeaderH + panelH;
    int arpY   = seqY + kSeqH;

    // Główne tło
    g.fillAll(juce::Colour(0xFF0D0D0D));

    // ── Nagłówek ─────────────────────────────────────────────────────────────
    juce::ColourGradient hdrGrad(
        juce::Colour(0xFF1E1E1E), 0.0f, 0.0f,
        juce::Colour(0xFF141414), 0.0f, (float)kHeaderH, false);
    g.setGradientFill(hdrGrad);
    g.fillRect(0, 0, w, kHeaderH);

    // Górna linia akcentu
    juce::ColourGradient accentLine(
        juce::Colour(0xFFCC3300), 0.0f, 0.0f,
        juce::Colour(0xFFFF6622), (float)w * 0.3f, 0.0f, false);
    g.setGradientFill(accentLine);
    g.fillRect(0, 0, w, 3);

    // Separator nagłówka
    g.setColour(juce::Colour(0xFF050505));
    g.fillRect(0, kHeaderH, w, 2);

    // ── Pasek ARP ─────────────────────────────────────────────────────────────
    g.setColour(juce::Colour(0xFF0A0A0A));
    g.fillRect(0, arpY, w, kArpH);

    // Górna linia separatora arpa
    g.setColour(juce::Colour(0xFF1C1C1C));
    g.fillRect(0, arpY, w, 1);

    // Akcentowana linia na dole paska arpa
    g.setColour(juce::Colour(0xFF1A1A1A));
    g.fillRect(0, arpY + kArpH - 1, w, 1);

    // Etykiety sekcji w pasku ARP
    g.setFont(juce::FontOptions(8.5f).withStyle("Bold"));
    int by  = arpY + 4;
    int btnH = kArpH - 8;

    auto drawSectionLabel = [&](const char* txt, int x, int sectionW) {
        g.setColour(juce::Colour(0xFF444444));
        g.drawText(txt, x, arpY, sectionW, 10, juce::Justification::centred, false);
    };
    drawSectionLabel("PATTERN",  68,  165);
    drawSectionLabel("BPM",      245, 120);
    drawSectionLabel("RATE",     378, 165);
    drawSectionLabel("OCTAVES",  556,  96);
}

// ── Rozmieszczenie ────────────────────────────────────────────────────────────

void PluginEditor::resized() {
    int w      = getWidth();
    int h      = getHeight();
    int btnH   = kHeaderH - 12;
    int y0     = 6;

    // ── Nagłówek ─────────────────────────────────────────────────────────────
    titleLabel_  .setBounds( 10, y0, 110, kHeaderH - 10);
    prevBtn_     .setBounds(125, y0,  28, btnH);
    presetBox_   .setBounds(155, y0, 230, btnH);
    nextBtn_     .setBounds(390, y0,  28, btnH);
    saveBtn_     .setBounds(422, y0,  55, btnH);
    deleteBtn_   .setBounds(481, y0,  40, btnH);
    octDownBtn_  .setBounds(530, y0,  34, btnH);
    octLabel_    .setBounds(566, y0,  54, btnH);
    octUpBtn_    .setBounds(622, y0,  34, btnH);
    perfLabel_   .setBounds(660, y0, w - 770, btnH);
    audioBtn_    .setBounds(w - 105, y0, 98, btnH);

    // ── Panel + SEQ + pasek ARP + klawiatura ──────────────────────────────────
    int panelH = h - kHeaderH - kSeqH - kArpH - kKeyboardH;
    int seqY   = kHeaderH + panelH;
    int arpY   = seqY + kSeqH;

    panel_   .setBounds(0, kHeaderH, w, panelH);
    seqView_ .setBounds(0, seqY,     w, kSeqH);
    keyboard_.setBounds(0, arpY + kArpH, w, kKeyboardH);

    // ── Rozmieszczenie elementów paska ARP ───────────────────────────────────
    int by   = arpY + 6;
    int abH  = kArpH - 12;   // wysokość przycisku w pasku

    // [ARP ON/OFF]
    arpOnBtn_  .setBounds(10,  by, 52, abH);

    // Wzorzec (4 przyciski × 38px, gap 3)
    int px = 68;
    arpUpBtn_ .setBounds(px,       by, 38, abH);
    arpDnBtn_ .setBounds(px + 41,  by, 38, abH);
    arpUdBtn_ .setBounds(px + 82,  by, 38, abH);
    arpRndBtn_.setBounds(px + 123, by, 38, abH);  // end=229

    // BPM
    int bx = 245;
    bpmLabel_ .setBounds(bx,      by, 72, abH);
    bpmDnBtn_ .setBounds(bx + 75, by, 24, abH);
    bpmUpBtn_ .setBounds(bx + 102,by, 24, abH);  // end=371

    // Rate (4 przyciski × 38px)
    int rx = 378;
    rate4Btn_ .setBounds(rx,       by, 38, abH);
    rate8Btn_ .setBounds(rx + 41,  by, 38, abH);
    rate16Btn_.setBounds(rx + 82,  by, 38, abH);
    rate32Btn_.setBounds(rx + 123, by, 38, abH);  // end=539

    // Oktawy (3 przyciski × 28px)
    int ox = 556;
    oct1Btn_.setBounds(ox,      by, 28, abH);
    oct2Btn_.setBounds(ox + 31, by, 28, abH);
    oct3Btn_.setBounds(ox + 62, by, 28, abH);
}

// ── Timer ─────────────────────────────────────────────────────────────────────

void PluginEditor::timerCallback() {
    perfLabel_.setText(
        "Glosy: " + juce::String(engine_.activeVoices()) + "/32",
        juce::dontSendNotification);
}

// ── Klawiatura komputerowa ────────────────────────────────────────────────────

void PluginEditor::updateOctaveLabel() {
    // Pokaż aktualną bazę: C4 + shift → C(4+shift)
    int baseOctave = 4 + octaveShift_;
    juce::String lbl = "C" + juce::String(baseOctave) + " [Z]";
    octLabel_.setText(lbl, juce::dontSendNotification);
}

bool PluginEditor::keyPressed(const juce::KeyPress& key) {
    int kc = key.getKeyCode();
    if (heldKeys_.count(kc)) return false;  // ignoruj key-repeat

    int base = keyCodeToMidi(kc);
    if (base < 0) return false;

    int note = std::clamp(base + octaveShift_ * 12, 0, 127);
    heldKeys_[kc] = note;
    engine_.noteOn(note, 0.8f);
    return true;
}

bool PluginEditor::keyStateChanged(bool /*isKeyDown*/) {
    for (auto it = heldKeys_.begin(); it != heldKeys_.end(); ) {
        if (!juce::KeyPress::isKeyCurrentlyDown(it->first)) {
            engine_.noteOff(it->second);
            it = heldKeys_.erase(it);
        } else {
            ++it;
        }
    }
    return false;
}

// ── Presety ───────────────────────────────────────────────────────────────────

void PluginEditor::loadPreset(int index) {
    auto* p = presets_.getPreset(index);
    if (!p) return;
    currentPreset_ = index;

    auto plist = panel_.paramList();
    std::vector<float> vals(plist.size(), 0.5f);
    for (auto& pv : p->params)
        for (int i = 0; i < (int)plist.size(); ++i)
            if (plist[i].first == pv.moduleId && plist[i].second == pv.paramIdx)
                { vals[i] = pv.value; break; }

    panel_.setValues(vals);
    for (auto& pv : p->params)
        engine_.setModuleParam(pv.moduleId, pv.paramIdx, pv.value);

    // Wczytaj dane sekwencera jeśli preset je zawiera
    if (p->hasSeq) {
        std::vector<std::pair<int,bool>> steps;
        steps.reserve(p->seqPattern.size());
        for (auto& s : p->seqPattern)
            steps.push_back({ s.note, s.active });
        seqView_.applyState(p->seqBPM, p->seqDiv, p->seqSteps, steps);
    }
}

void PluginEditor::saveCurrentPreset() {
    auto* dlg = new juce::AlertWindow("Zapisz preset", "Nazwa presetu:",
                                       juce::MessageBoxIconType::NoIcon);
    auto* cur = presets_.getPreset(currentPreset_);
    dlg->addTextEditor("name", (cur && !cur->isFactory) ? cur->name : "Moj preset");
    dlg->addButton("Zapisz", 1);
    dlg->addButton("Anuluj", 0);

    dlg->enterModalState(true, juce::ModalCallbackFunction::create([this, dlg](int r) {
        if (r == 1) {
            juce::String name = dlg->getTextEditorContents("name").trim();
            if (name.isEmpty()) { delete dlg; return; }

            auto plist = panel_.paramList();
            auto vals  = panel_.getValues();
            std::vector<PresetManager::ParamValue> pvs;
            for (int i = 0; i < (int)plist.size(); ++i)
                pvs.push_back({ plist[i].first, plist[i].second, vals[i] });
            presets_.savePreset(name, pvs);

            presetBox_.clear(juce::dontSendNotification);
            auto names = presets_.getNames();
            for (int i = 0; i < names.size(); ++i)
                presetBox_.addItem(names[i], i + 1);

            auto* saved = presets_.getPreset(name);
            if (saved)
                for (int i = 0; i < presets_.getCount(); ++i)
                    if (presets_.getPreset(i) == saved) {
                        currentPreset_ = i;
                        presetBox_.setSelectedId(i + 1, juce::dontSendNotification);
                        break;
                    }
        }
        delete dlg;
    }), false);
}

// ── Arpeggiator helpers ───────────────────────────────────────────────────────

void PluginEditor::updateBpmLabel() {
    bpmLabel_.setText("BPM: " + juce::String((int)arpBPM_),
                      juce::dontSendNotification);
}

// Aktywny przycisk wzorca — pomarańczowy, inne szare
void PluginEditor::selectArpPattern(synth::Arpeggiator::Pattern p) {
    arpPattern_ = p;
    engine_.setArpPattern(p);
    const juce::Colour active { 0xFF1E3A10 };
    const juce::Colour atext  { 0xFF88FF55 };
    const juce::Colour idle   { 0xFF1A1A1A };
    const juce::Colour itext  { 0xFF888888 };

    auto hi = [&](juce::TextButton& b, bool on) {
        b.setColour(juce::TextButton::buttonColourId, on ? active : idle);
        b.setColour(juce::TextButton::textColourOffId, on ? atext : itext);
    };
    hi(arpUpBtn_,  p == synth::Arpeggiator::Pattern::Up);
    hi(arpDnBtn_,  p == synth::Arpeggiator::Pattern::Down);
    hi(arpUdBtn_,  p == synth::Arpeggiator::Pattern::UpDown);
    hi(arpRndBtn_, p == synth::Arpeggiator::Pattern::Random);
    repaint();
}

void PluginEditor::selectArpRate(int division) {
    arpRate_ = division;
    engine_.setArpDivision(division);
    const juce::Colour active { 0xFF1A2A3A };
    const juce::Colour atext  { 0xFF55AAFF };
    const juce::Colour idle   { 0xFF1A1A1A };
    const juce::Colour itext  { 0xFF888888 };

    auto hi = [&](juce::TextButton& b, bool on) {
        b.setColour(juce::TextButton::buttonColourId, on ? active : idle);
        b.setColour(juce::TextButton::textColourOffId, on ? atext : itext);
    };
    hi(rate4Btn_,  division == 4);
    hi(rate8Btn_,  division == 8);
    hi(rate16Btn_, division == 16);
    hi(rate32Btn_, division == 32);
    repaint();
}

void PluginEditor::selectArpOct(int oct) {
    arpOct_ = oct;
    engine_.setArpOctaves(oct);
    const juce::Colour active { 0xFF2A1A3A };
    const juce::Colour atext  { 0xFFCC77FF };
    const juce::Colour idle   { 0xFF1A1A1A };
    const juce::Colour itext  { 0xFF888888 };

    auto hi = [&](juce::TextButton& b, bool on) {
        b.setColour(juce::TextButton::buttonColourId, on ? active : idle);
        b.setColour(juce::TextButton::textColourOffId, on ? atext : itext);
    };
    hi(oct1Btn_, oct == 1);
    hi(oct2Btn_, oct == 2);
    hi(oct3Btn_, oct == 3);
    repaint();
}

// ── Audio settings ────────────────────────────────────────────────────────────

void PluginEditor::openAudioSettings() {
#if JUCE_STANDALONE_APPLICATION
    auto* holder = juce::StandalonePluginHolder::getInstance();
    if (!holder) return;

    auto* selector = new juce::AudioDeviceSelectorComponent(
        holder->deviceManager, 0, 0, 2, 2, false, false, false, false);
    selector->setSize(500, 400);

    juce::DialogWindow::LaunchOptions opts;
    opts.content.setOwned(selector);
    opts.dialogTitle                  = "Ustawienia audio";
    opts.dialogBackgroundColour       = juce::Colour(0xFF1A1A2A);
    opts.componentToCentreAround      = this;
    opts.escapeKeyTriggersCloseButton = true;
    opts.useNativeTitleBar            = false;
    opts.resizable                    = false;
    opts.launchAsync();
#else
    juce::AlertWindow::showMessageBoxAsync(
        juce::MessageBoxIconType::InfoIcon,
        "Ustawienia audio", "Dostepne tylko w wersji Standalone.");
#endif
}
