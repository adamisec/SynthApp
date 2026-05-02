#include "SimplePanelView.h"

// Module IDs z AudioEngine::prepareToPlay:
//  0=KeyNote  1=OscA  2=OscB  3=Mix4to1  4=FltNord  5=LfoA
//  6=EnvADSR  7=VCA   8=FxChorus  9=FxReverb  10=Out

struct KnobSpec { int mod, param; const char* lbl; float init; };

static void addSection(std::vector<SimplePanelView::Section>& sections,
                       const char* name,
                       juce::Colour hdr, juce::Colour accent,
                       std::initializer_list<KnobSpec> specs)
{
    SimplePanelView::Section sec;
    sec.name         = name;
    sec.headerColour = hdr;
    sec.accentColour = accent;
    for (auto& s : specs) {
        SimplePanelView::KnobEntry ke;
        ke.moduleId = s.mod;
        ke.paramIdx = s.param;
        ke.label    = s.lbl;
        ke.initVal  = s.init;
        sec.knobs.push_back(std::move(ke));
    }
    sections.push_back(std::move(sec));
}

// ── Konstruktor ───────────────────────────────────────────────────────────────

SimplePanelView::SimplePanelView() {
    addSection(sections_, "OSCYLATORY",
               juce::Colour(0xFFB84000), juce::Colour(0xFFFF7733), {
        { 2, 1, "DETUNE",   0.555f },
        { 1, 2, "KSZTALT",  0.5f   },
        { 3, 0, "POZIOM A", 0.7f   },
        { 3, 1, "POZIOM B", 0.7f   },
    });
    addSection(sections_, "FILTR",
               juce::Colour(0xFF0077BB), juce::Colour(0xFF33AAFF), {
        { 4, 0, "CUTOFF",   0.45f },
        { 4, 1, "REZONANS", 0.35f },
        { 5, 2, "LFO MOD",  0.22f },
    });
    addSection(sections_, "KOPERTA",
               juce::Colour(0xFF007733), juce::Colour(0xFF44FF88), {
        { 6, 0, "ATAK",    0.25f },
        { 6, 1, "DECAY",   0.3f  },
        { 6, 2, "SUSTAIN", 0.75f },
        { 6, 3, "RELEASE", 0.55f },
    });
    addSection(sections_, "LFO",
               juce::Colour(0xFF7711BB), juce::Colour(0xFFBB55FF), {
        { 5, 0, "SZYBKOSC", 0.2f },
        { 5, 1, "KSZTALT",  0.0f },
    });
    addSection(sections_, "EFEKTY",
               juce::Colour(0xFF007766), juce::Colour(0xFF33FFCC), {
        { 8, 3, "CHORUS",   0.5f  },
        { 9, 3, "REVERB",   0.45f },
        { 9, 0, "ROZMIAR",  0.8f  },
        {10, 0, "GLOSNOSC", 0.95f },
    });

    // ── Tworzenie pokręteł ────────────────────────────────────────────────────
    for (auto& sec : sections_) {
        for (auto& ke : sec.knobs) {
            auto k = std::make_unique<KnobView>(ke.label);
            k->setAccentColour(sec.accentColour);
            k->setValue(ke.initVal, juce::dontSendNotification);
            k->setDefaultValue(ke.initVal);
            k->setMouseClickGrabsKeyboardFocus(false);
            k->setWantsKeyboardFocus(false);

            int modId    = ke.moduleId;
            int paramIdx = ke.paramIdx;
            k->onValueChanged = [this, modId, paramIdx](float v) {
                if (onParamChanged) onParamChanged(modId, paramIdx, v);
            };
            addAndMakeVisible(k.get());
            ke.knob = std::move(k);
        }
    }
}

// ── Odczyt / zapis parametrów (presety) ──────────────────────────────────────

std::vector<std::pair<int,int>> SimplePanelView::paramList() const {
    std::vector<std::pair<int,int>> list;
    for (auto& sec : sections_)
        for (auto& ke : sec.knobs)
            list.push_back({ ke.moduleId, ke.paramIdx });
    return list;
}

std::vector<float> SimplePanelView::getValues() const {
    std::vector<float> vals;
    for (auto& sec : sections_)
        for (auto& ke : sec.knobs)
            vals.push_back(ke.knob->getValue());
    return vals;
}

void SimplePanelView::setValues(const std::vector<float>& values) {
    int idx = 0;
    for (auto& sec : sections_)
        for (auto& ke : sec.knobs) {
            if (idx >= (int)values.size()) return;
            ke.knob->setValue(values[idx++], juce::dontSendNotification);
        }
}

// ── Rysowanie ─────────────────────────────────────────────────────────────────

void SimplePanelView::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xFF0D0D0D));

    int numSec  = (int)sections_.size();
    int secW    = getWidth() / numSec;
    int headerH = 36;

    for (int s = 0; s < numSec; ++s) {
        auto& sec = sections_[s];
        int   x   = s * secW;

        // Tło sekcji
        g.setColour(juce::Colour(0xFF111111));
        g.fillRect(x, headerH, secW, getHeight() - headerH);

        // Nagłówek sekcji — gradient pionowy z kolorem akcentu
        juce::ColourGradient hdr(
            sec.headerColour.brighter(0.15f), (float)x, 0.0f,
            sec.headerColour.darker(0.4f),    (float)x, (float)headerH, false);
        g.setGradientFill(hdr);
        g.fillRect(x, 0, secW, headerH);

        // Górna linia blasku nagłówka
        g.setColour(juce::Colour(0x22FFFFFF));
        g.fillRect(x, 0, secW, 2);

        // Linia akcentu pod nagłówkiem
        g.setColour(sec.accentColour.withAlpha(0.5f));
        g.fillRect(x, headerH - 2, secW, 2);

        // Nazwa sekcji
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(11.5f).withStyle("Bold"));
        g.drawText(sec.name, x + 4, 0, secW - 8, headerH,
                   juce::Justification::centred, false);

        // Separator pionowy
        if (s > 0) {
            g.setColour(juce::Colour(0xFF050505));
            g.drawLine((float)x, 0.0f, (float)x, (float)getHeight(), 2.0f);
            g.setColour(juce::Colour(0xFF222222));
            g.drawLine((float)(x + 1), (float)headerH, (float)(x + 1),
                       (float)getHeight(), 0.5f);
        }
    }

    // Dolna linia oddzielająca od klawiatury
    g.setColour(juce::Colour(0xFF050505));
    g.drawLine(0.0f, (float)(getHeight() - 1),
               (float)getWidth(), (float)(getHeight() - 1), 1.0f);
}

// ── Rozmieszczenie pokręteł — układ 2-kolumnowy ───────────────────────────────

void SimplePanelView::resized() {
    const int headerH = 36;
    const int cols    = 2;
    const int padX    = 6;
    const int padY    = 10;

    int numSec   = (int)sections_.size();
    int secW     = getWidth() / numSec;
    int contentW = secW - padX * 2;
    int contentH = getHeight() - headerH - padY * 2;

    int slotW    = contentW / cols;
    int slotH    = contentH / cols;  // 2 wiersze

    for (int s = 0; s < numSec; ++s) {
        auto& sec  = sections_[s];
        int   secX = s * secW;
        int   n    = (int)sec.knobs.size();

        int rows    = (n + cols - 1) / cols;
        int knobSz  = std::min(slotW, slotH) - 6;

        for (int i = 0; i < n; ++i) {
            int row = i / cols;
            int col = i % cols;

            // Wyśrodkuj ostatni wiersz jeśli niepełny
            int rowItems = (row == rows - 1) ? (n - row * cols) : cols;
            int colShift = (cols - rowItems) * slotW / 2;

            int cx = secX + padX + colShift + col * slotW + (slotW - knobSz) / 2;
            int cy = headerH + padY + row * slotH + (slotH - knobSz) / 2;

            sec.knobs[i].knob->setBounds(cx, cy, knobSz, knobSz);
        }
    }
}
