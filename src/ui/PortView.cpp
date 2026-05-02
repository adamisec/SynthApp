#include "PortView.h"

PortView::PortView(const synth::PortInfo& info, int moduleId, int portIndex)
    : info_(info), moduleId_(moduleId), portIndex_(portIndex) {
    setSize(16, 16);
}

juce::Colour PortView::colourFor(synth::SignalType t) {
    switch (t) {
        case synth::SignalType::Audio: return juce::Colour(0xFFE8C840);  // żółty
        case synth::SignalType::CV:    return juce::Colour(0xFF4080E8);  // niebieski
        case synth::SignalType::Gate:  return juce::Colour(0xFFE87820);  // pomarańczowy
        default:                       return juce::Colours::white;
    }
}

juce::Point<float> PortView::centrePosInParent() const {
    return getParentComponent()
        ? getBoundsInParent().toFloat().getCentre()
        : getLocalBounds().toFloat().getCentre();
}

void PortView::paint(juce::Graphics& g) {
    auto c = colourFor(info_.type);
    float r = getWidth() * 0.38f;
    float cx = getWidth()  * 0.5f;
    float cy = getHeight() * 0.5f;

    // Zewnętrzny pierścień
    g.setColour(juce::Colour(0xFF222222));
    g.fillEllipse(cx - r - 2, cy - r - 2, (r + 2) * 2, (r + 2) * 2);

    // Kolor portu (połączony = pełny, niepołączony = zarys)
    if (connected_) {
        g.setColour(c);
        g.fillEllipse(cx - r, cy - r, r * 2, r * 2);
    } else {
        g.setColour(c.withAlpha(0.5f));
        g.drawEllipse(cx - r, cy - r, r * 2, r * 2, 1.5f);
    }

    // Małe kółko w środku (jak prawdziwy jack)
    g.setColour(juce::Colour(0xFF111111));
    g.fillEllipse(cx - r * 0.35f, cy - r * 0.35f, r * 0.7f, r * 0.7f);
}

void PortView::mouseDown(const juce::MouseEvent& /*e*/) {
    if (onCableStart) onCableStart(this);
}
