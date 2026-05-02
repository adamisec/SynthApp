#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

// ── KnobView — nowoczesne pokrętło 270° ──────────────────────────────────────
// - Drag góra/dół → zmiana wartości
// - Shift + drag  → precyzja 10x wolniej
// - Double-click  → reset do wartości domyślnej
// - Prawy klik    → context menu (Wpisz wartość / Reset)
class KnobView : public juce::Component,
                 public juce::SettableTooltipClient {
public:
    explicit KnobView(const juce::String& label = "");

    void  setValue(float v, juce::NotificationType notify = juce::sendNotification);
    float getValue() const { return value_; }

    void setDefaultValue(float v)          { defaultValue_ = v; }
    void setLabel(const juce::String& l)   { label_ = l; repaint(); }
    void setAccentColour(juce::Colour c)   { accentColour_ = c; repaint(); }

    std::function<void(float)> onValueChanged;

    void paint(juce::Graphics& g) override;
    void resized() override {}

    void mouseDown       (const juce::MouseEvent& e) override;
    void mouseDrag       (const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

private:
    float        value_        = 0.5f;
    float        defaultValue_ = 0.5f;
    float        dragStart_    = 0.5f;
    int          dragStartY_   = 0;
    juce::String label_;
    juce::Colour accentColour_ { 0xFF22AAFF };

    static constexpr float kArcStart = 225.0f;
    static constexpr float kArcRange = 270.0f;
};
