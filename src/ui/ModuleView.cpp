#include "ModuleView.h"
#include <algorithm>

// ── Kolor kategorii modułu wg nazwy ──────────────────────────────────────────
juce::Colour ModuleView::categoryColour(const juce::String& name) {
    if (name.startsWith("Osc") || name == "Noise")
        return juce::Colour(0xFF8B1A1A);  // bordowy — oscylatory
    if (name.startsWith("Flt"))
        return juce::Colour(0xFF1A3A6B);  // ciemnoniebieski — filtry
    if (name.startsWith("Env") || name.startsWith("Lfo"))
        return juce::Colour(0xFF1A5C2A);  // ciemnozielony — koperty/LFO
    if (name.startsWith("Fx"))
        return juce::Colour(0xFF3A1A6B);  // ciemnofioletowy — efekty
    if (name.startsWith("Seq") || name.startsWith("Clk"))
        return juce::Colour(0xFF4A3A10);  // ciemnobrązowy — sekwencer
    return juce::Colour(0xFF3A3A3A);      // grafitowy — reszta
}

// ── Konstruktor ───────────────────────────────────────────────────────────────
ModuleView::ModuleView(synth::Module& module, int moduleId)
    : module_(module), moduleId_(moduleId) {

    int numParams = module.getNumParams();
    int numInputs = 0, numOutputs = 0;

    auto ports = module.getPorts();
    for (auto& p : ports) {
        if (p.isOutput) ++numOutputs; else ++numInputs;
    }

    // Rozmiar komponentu
    int cols     = std::max(numParams, 1);
    int w        = kPadding * 2 + cols * (kKnobSize + kPadding);
    int portRows = std::max(numInputs, numOutputs);
    int h        = kHeaderH + kPadding
                 + numParams * (kKnobSize + kPadding + 14)
                 + portRows  * (kPortSize + kPadding)
                 + kPadding;

    setSize(std::max(w, 90), h);

    // Knobs
    for (int i = 0; i < numParams; ++i) {
        auto* k = new KnobView(juce::String("P") + juce::String(i));
        k->setValue(module.getParam(i), juce::dontSendNotification);
        k->onValueChanged = [this, i](float v) {
            module_.setParam(i, v);
            if (onParamChanged) onParamChanged(moduleId_, i, v);
        };
        addAndMakeVisible(k);
        knobs_.emplace_back(k);
    }

    // Ports
    int inIdx = 0, outIdx = 0;
    for (int i = 0; i < (int)ports.size(); ++i) {
        auto* pv = new PortView(ports[i], moduleId_, i);
        pv->onCableStart = [this](PortView* p) {
            if (onCableStart) onCableStart(p);
        };
        addAndMakeVisible(pv);
        ports_.emplace_back(pv);
        (void)inIdx; (void)outIdx;
    }
}

// ── Layout ─────────────────────────────────────────────────────────────────────
void ModuleView::resized() {
    int y = kHeaderH + kPadding;
    int x = kPadding;

    // Knobs w rzędzie
    for (auto& k : knobs_) {
        k->setBounds(x, y, kKnobSize, kKnobSize + 14);
        x += kKnobSize + kPadding;
        if (x + kKnobSize > getWidth() - kPadding) {
            x = kPadding;
            y += kKnobSize + kPadding + 14;
        }
    }

    // Porty: wejścia po lewej, wyjścia po prawej
    if (!knobs_.empty()) y += kKnobSize + kPadding + 14;

    int inY = y, outY = y;
    for (auto& pv : ports_) {
        if (pv->portInfo().isOutput) {
            pv->setBounds(getWidth() - kPadding - kPortSize, outY, kPortSize, kPortSize);
            outY += kPortSize + kPadding;
        } else {
            pv->setBounds(kPadding, inY, kPortSize, kPortSize);
            inY += kPortSize + kPadding;
        }
    }
}

// ── Renderowanie ──────────────────────────────────────────────────────────────
void ModuleView::paint(juce::Graphics& g) {
    auto name   = juce::String(module_.getName().c_str());
    auto colour = categoryColour(name);

    // Tło modułu
    g.setColour(juce::Colour(0xFF252525));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);

    // Pasek tytułu
    g.setColour(colour);
    g.fillRoundedRectangle(0.0f, 0.0f, (float)getWidth(), (float)kHeaderH, 5.0f);
    g.fillRect(0, kHeaderH / 2, getWidth(), kHeaderH / 2);

    // Nazwa modułu
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(10.5f).withStyle("Bold"));
    g.drawText(name, 4, 0, getWidth() - 8, kHeaderH,
               juce::Justification::centredLeft, true);

    // Obramowanie
    g.setColour(juce::Colour(0xFF444444));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 5.0f, 1.0f);
}

// ── Drag modułu ───────────────────────────────────────────────────────────────
void ModuleView::mouseDown(const juce::MouseEvent& e) {
    if (e.y > kHeaderH) return;  // tylko header drag
    dragStart_ = getPosition();
}

void ModuleView::mouseDrag(const juce::MouseEvent& e) {
    if (e.mouseWasDraggedSinceMouseDown() && e.y <= kHeaderH + 10) {
        auto parent = getParentComponent();
        if (!parent) return;
        auto newPos = dragStart_ + e.getOffsetFromDragStart();
        newPos.x = std::clamp(newPos.x, 0, parent->getWidth()  - getWidth());
        newPos.y = std::clamp(newPos.y, 0, parent->getHeight() - getHeight());
        setTopLeftPosition(newPos);
        if (parent) parent->repaint();
    }
}
