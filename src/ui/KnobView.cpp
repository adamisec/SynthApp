#include "KnobView.h"
#include <algorithm>
#include <cmath>

KnobView::KnobView(const juce::String& label)
    : label_(label) {}

void KnobView::setValue(float v, juce::NotificationType notify) {
    v = std::clamp(v, 0.0f, 1.0f);
    if (value_ == v) return;
    value_ = v;
    repaint();
    if (notify == juce::sendNotification && onValueChanged)
        onValueChanged(value_);
}

void KnobView::paint(juce::Graphics& g) {
    const float labelH   = 14.0f;
    const float halfPi   = juce::MathConstants<float>::halfPi;
    const float startRad = juce::MathConstants<float>::pi * (kArcStart / 180.0f);
    const float arcRad   = juce::MathConstants<float>::pi * (kArcRange / 180.0f);

    auto full    = getLocalBounds().toFloat();
    auto area    = full.withBottom(full.getBottom() - labelH).reduced(4.0f);
    float cx     = area.getCentreX();
    float cy     = area.getCentreY();
    float r      = std::min(area.getWidth(), area.getHeight()) * 0.44f;
    float trackR = r + 4.0f;

    // ── Ścieżka tła łuku ────────────────────────────────────────────────────
    {
        juce::Path bg;
        bg.addCentredArc(cx, cy, trackR, trackR, 0.0f,
                         startRad, startRad + arcRad, true);
        g.setColour(juce::Colour(0xFF232323));
        g.strokePath(bg, juce::PathStrokeType(4.0f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // ── Łuk wartości (kolor akcentu) ────────────────────────────────────────
    if (value_ > 0.001f) {
        juce::Path fill;
        fill.addCentredArc(cx, cy, trackR, trackR, 0.0f,
                           startRad, startRad + arcRad * value_, true);
        g.setColour(accentColour_);
        g.strokePath(fill, juce::PathStrokeType(4.0f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Świecący punkt na końcu łuku
        float ea = startRad + arcRad * value_ - halfPi;
        float ex = cx + std::cos(ea) * trackR;
        float ey = cy + std::sin(ea) * trackR;
        g.setColour(accentColour_.withAlpha(0.35f));
        g.fillEllipse(ex - 5.5f, ey - 5.5f, 11.0f, 11.0f);
        g.setColour(accentColour_.brighter(0.6f));
        g.fillEllipse(ex - 2.8f, ey - 2.8f, 5.6f, 5.6f);
    }

    // ── Cień pokrętła ───────────────────────────────────────────────────────
    g.setColour(juce::Colour(0x55000000));
    g.fillEllipse(cx - r + 1.5f, cy - r + 3.0f, r * 2.0f, r * 2.0f);

    // ── Pierścień zewnętrzny ────────────────────────────────────────────────
    g.setColour(juce::Colour(0xFF0D0D0D));
    g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);

    // ── Ciało pokrętła — gradient radialny ──────────────────────────────────
    juce::ColourGradient bodyGrad(
        juce::Colour(0xFF505050), cx - r * 0.3f, cy - r * 0.3f,
        juce::Colour(0xFF1A1A1A), cx + r * 0.4f, cy + r * 0.5f, true);
    g.setGradientFill(bodyGrad);
    float ir = r - 2.5f;
    g.fillEllipse(cx - ir, cy - ir, ir * 2.0f, ir * 2.0f);

    // ── Połysk (górna poświata) ──────────────────────────────────────────────
    g.setColour(juce::Colour(0x16FFFFFF));
    g.fillEllipse(cx - ir * 0.55f, cy - ir * 0.82f, ir * 1.0f, ir * 0.52f);

    // ── Wskaźnik — biała kropka przy krawędzi ──────────────────────────────
    float angle  = startRad + arcRad * value_ - halfPi;
    float dotR   = ir * 0.65f;
    float dotX   = cx + std::cos(angle) * dotR;
    float dotY   = cy + std::sin(angle) * dotR;
    g.setColour(juce::Colour(0xCCFFFFFF));
    g.fillEllipse(dotX - 2.8f, dotY - 2.8f, 5.6f, 5.6f);

    // ── Etykieta pod pokrętłem ──────────────────────────────────────────────
    if (label_.isNotEmpty()) {
        g.setColour(juce::Colour(0xFF888888));
        g.setFont(juce::FontOptions(9.0f));
        g.drawText(label_,
                   0, (int)(full.getBottom() - labelH),
                   getWidth(), (int)labelH,
                   juce::Justification::centred, true);
    }
}

void KnobView::mouseDown(const juce::MouseEvent& e) {
    if (e.mods.isRightButtonDown()) {
        juce::PopupMenu menu;
        menu.addItem(1, "Reset do domyślnej");
        menu.addItem(2, "Wpisz wartość...");
        menu.showMenuAsync(juce::PopupMenu::Options{}, [this](int result) {
            if (result == 1) setValue(defaultValue_);
            if (result == 2) {
                auto* dlg = new juce::AlertWindow("Wartość", "Podaj wartość 0..1:", juce::MessageBoxIconType::NoIcon);
                dlg->addTextEditor("val", juce::String(value_, 3));
                dlg->addButton("OK",     1);
                dlg->addButton("Anuluj", 0);
                dlg->enterModalState(true, juce::ModalCallbackFunction::create([this, dlg](int r) {
                    if (r == 1) {
                        float v = dlg->getTextEditorContents("val").getFloatValue();
                        setValue(v);
                    }
                    delete dlg;
                }), false);
            }
        });
        return;
    }
    dragStart_  = value_;
    dragStartY_ = e.getMouseDownY();
}

void KnobView::mouseDrag(const juce::MouseEvent& e) {
    if (e.mods.isRightButtonDown()) return;
    float delta = (dragStartY_ - e.getPosition().y) * 0.005f;
    if (e.mods.isShiftDown()) delta *= 0.01f;
    setValue(dragStart_ + delta);
}

void KnobView::mouseDoubleClick(const juce::MouseEvent& /*e*/) {
    setValue(defaultValue_);
}
