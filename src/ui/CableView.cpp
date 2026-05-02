#include "CableView.h"
#include "ui/PortView.h"

CableView::CableView() {
    setInterceptsMouseClicks(false, false);  // kliknięcia przechodzą do modułów
}

void CableView::addCable(const Cable& c) {
    cables_.push_back(c);
    repaint();
}

void CableView::removeCable(int fromModule, int fromPort, int toModule, int toPort) {
    cables_.erase(std::remove_if(cables_.begin(), cables_.end(),
        [&](const Cable& c) {
            return c.fromModule == fromModule && c.fromPort == fromPort
                && c.toModule   == toModule   && c.toPort   == toPort;
        }), cables_.end());
    repaint();
}

void CableView::clearAllCables() {
    cables_.clear();
    repaint();
}

void CableView::setDragCable(juce::Point<float> from, juce::Point<float> to,
                              synth::SignalType type) {
    dragging_ = true;
    dragFrom_ = from;
    dragTo_   = to;
    dragType_ = type;
    repaint();
}

void CableView::drawCable(juce::Graphics& g,
                           juce::Point<float> a, juce::Point<float> b,
                           juce::Colour c, float thickness) const {
    // Zwisanie kabla — naturalne opadanie (fizyka sprężyny)
    float slack = std::min(80.0f, std::abs(b.y - a.y) * 0.4f + 30.0f);
    float midY  = std::max(a.y, b.y) + slack;

    juce::Point<float> ctrl1 { a.x, midY };
    juce::Point<float> ctrl2 { b.x, midY };

    juce::Path path;
    path.startNewSubPath(a);
    path.cubicTo(ctrl1, ctrl2, b);

    // Cień kabla (głębia)
    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.strokePath(path, juce::PathStrokeType(thickness + 2.0f,
        juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Właściwy kabel
    g.setColour(c.withAlpha(0.88f));
    g.strokePath(path, juce::PathStrokeType(thickness,
        juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void CableView::paint(juce::Graphics& g) {
    // Istniejące kable
    for (auto& cable : cables_) {
        auto colour = PortView::colourFor(cable.signalType);
        drawCable(g, cable.from, cable.to, colour, 2.5f);
    }

    // Kabel w trakcie ciągnięcia (przerywany / jaśniejszy)
    if (dragging_) {
        auto colour = PortView::colourFor(dragType_);
        drawCable(g, dragFrom_, dragTo_, colour.brighter(0.4f), 2.0f);
    }
}
