#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "engine/ModuleGraph.h"
#include "modules/ModuleRegistry.h"
#include "ui/ModuleView.h"
#include "ui/CableView.h"
#include "ui/SlotTabView.h"
#include <vector>
#include <memory>

// ── PanelView — główny panel G2 z modułami i kablami ─────────────────────────
// Zawiera:
//  - Pasek slotów A/B/C/D (góra)
//  - Obszar canvas z modułami (przeciągane)
//  - Nakładka kablowa (CableView) na wierzchu canvas
//  - Pasek statusu (dół)
class PanelView : public juce::Component {
public:
    explicit PanelView(synth::ModuleGraph& graph);
    ~PanelView() override;

    // Dodaj moduł do widoku (po dodaniu do grafu)
    void addModuleView(synth::Module& m, int moduleId,
                       int x = 20, int y = 20);

    // Dodaj kabel do widoku (po dodaniu do grafu)
    void addCableView(int fromModule, int fromPort,
                      int toModule,   int toPort,
                      synth::SignalType type);

    void removeAllModuleViews();

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Callback gdy parametr jest zmieniany z UI
    std::function<void(int moduleId, int paramIdx, float value)> onParamChanged;

private:
    void startCableDrag(PortView* port);
    void finishCableDrag(PortView* port);
    void cancelCableDrag();

    // Znajdź PortView w całym panelu po pozycji myszy
    PortView* findPortAt(juce::Point<int> pos) const;

    void mouseMove(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;

    void showAddModuleMenu(juce::Point<int> canvasPos);
    void spawnModule(const juce::String& name, juce::Point<int> canvasPos);
    void removeModuleView(int moduleId);

    synth::ModuleGraph& graph_;

    SlotTabView slotTabs_;
    juce::Component canvas_;   // kontener dla modułów
    CableView  cableOverlay_;  // nakładka kablowa

    std::vector<std::unique_ptr<ModuleView>> moduleViews_;

    // Stan ciągnięcia kabla
    PortView* dragStartPort_ = nullptr;

    static constexpr int kSlotBarH  = 28;
    static constexpr int kStatusBarH = 22;
};
