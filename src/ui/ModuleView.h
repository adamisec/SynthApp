#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "engine/Module.h"
#include "ui/KnobView.h"
#include "ui/PortView.h"
#include <vector>
#include <memory>
#include <functional>

// ── ModuleView — karta modułu na panelu ──────────────────────────────────────
// Rysuje: pasek tytułu (kolor kategorii), pokrętła (KnobView), porty (PortView).
// Można przenosić przez drag nagłówka.
class ModuleView : public juce::Component {
public:
    ModuleView(synth::Module& module, int moduleId);

    // Callback gdy parametr modułu się zmienił (ustawiany przez PanelView)
    std::function<void(int moduleId, int paramIdx, float value)> onParamChanged;
    // Callback dla kablów — deleguj do PortView
    std::function<void(PortView*)> onCableStart;

    synth::Module& module()   { return module_; }
    int            moduleId() const { return moduleId_; }

    const std::vector<std::unique_ptr<PortView>>& ports() const { return ports_; }

    static juce::Colour categoryColour(const juce::String& moduleName);

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

private:
    static constexpr int kHeaderH  = 18;
    static constexpr int kKnobSize = 42;
    static constexpr int kPortSize = 16;
    static constexpr int kPadding  = 6;

    synth::Module& module_;
    int            moduleId_;

    std::vector<std::unique_ptr<KnobView>> knobs_;
    std::vector<std::unique_ptr<PortView>> ports_;

    juce::Point<int> dragStart_;
};
