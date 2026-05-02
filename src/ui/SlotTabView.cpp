#include "SlotTabView.h"

SlotTabView::SlotTabView() {
    setSize(160, 28);
}

void SlotTabView::setActiveSlot(int s, juce::NotificationType n) {
    if (activeSlot_ == s) return;
    activeSlot_ = s;
    repaint();
    if (n == juce::sendNotification && onSlotChanged)
        onSlotChanged(activeSlot_);
}

void SlotTabView::paint(juce::Graphics& g) {
    const char* labels[] = { "A", "B", "C", "D" };
    int w = getWidth() / kNumSlots;

    for (int i = 0; i < kNumSlots; ++i) {
        bool active = (i == activeSlot_);
        juce::Rectangle<int> r(i * w, 0, w, getHeight());

        g.setColour(active ? juce::Colour(0xFF444444) : juce::Colour(0xFF222222));
        g.fillRect(r);

        g.setColour(active ? juce::Colours::white : juce::Colour(0xFF666666));
        g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
        g.drawText(labels[i], r, juce::Justification::centred, false);

        // Dolna linia aktywnej zakładki
        if (active) {
            g.setColour(juce::Colour(0xFF4488FF));
            g.fillRect(r.getX(), r.getBottom() - 2, r.getWidth(), 2);
        }

        // Separator
        g.setColour(juce::Colour(0xFF333333));
        g.drawVerticalLine(r.getRight(), 0, getHeight());
    }
}

void SlotTabView::mouseDown(const juce::MouseEvent& e) {
    int slot = e.x / (getWidth() / kNumSlots);
    setActiveSlot(std::clamp(slot, 0, kNumSlots - 1));
}
