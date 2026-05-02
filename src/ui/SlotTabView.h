#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

// ── SlotTabView — zakładki slotów A/B/C/D ────────────────────────────────────
class SlotTabView : public juce::Component {
public:
    std::function<void(int slotIndex)> onSlotChanged;

    SlotTabView();

    int  activeSlot() const { return activeSlot_; }
    void setActiveSlot(int s, juce::NotificationType n = juce::sendNotification);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;

private:
    static constexpr int kNumSlots = 4;
    int activeSlot_ = 0;
};
