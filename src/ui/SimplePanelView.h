#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "KnobView.h"
#include <functional>
#include <vector>
#include <memory>

// ── SimplePanelView — panel pokręteł syntezatora ─────────────────────────────
// 5 sekcji: OSCYLATORY | FILTR | KOPERTA | LFO | EFEKTY
// Układ 2×2 pokręteł na sekcję (lub 1×n dla mniejszych sekcji).
class SimplePanelView : public juce::Component {
public:
    // Wywołanie przy zmianie pokrętła: (moduleId, paramIdx, wartość 0..1)
    std::function<void(int, int, float)> onParamChanged;

    SimplePanelView();

    void paint(juce::Graphics& g) override;
    void resized()                override;

    // Odczyt / zapis wszystkich pokręteł (do presetów)
    std::vector<std::pair<int,int>> paramList() const;
    std::vector<float>              getValues() const;
    void setValues(const std::vector<float>& values);

    struct KnobEntry {
        int          moduleId  = 0;
        int          paramIdx  = 0;
        juce::String label;
        float        initVal   = 0.5f;
        std::unique_ptr<KnobView> knob;
    };

    struct Section {
        juce::String            name;
        juce::Colour            headerColour;
        juce::Colour            accentColour;
        std::vector<KnobEntry>  knobs;
    };

private:
    std::vector<Section> sections_;
};
