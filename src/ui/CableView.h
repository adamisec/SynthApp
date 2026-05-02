#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "engine/Module.h"
#include <vector>

// ── CableView — przezroczysta nakładka renderująca kable jako krzywe Beziera ─
// Leży na wierzchu PanelView i rysuje wszystkie połączenia.
// Obsługuje też ciągnięcie nowego kabla (podgląd w locie).
class CableView : public juce::Component {
public:
    struct Cable {
        juce::Point<float> from, to;
        synth::SignalType  signalType;
        int fromModule = -1, fromPort = -1;
        int toModule   = -1, toPort   = -1;
    };

    CableView();

    void addCable(const Cable& c);
    void removeCable(int fromModule, int fromPort, int toModule, int toPort);
    void clearAllCables();

    const std::vector<Cable>& cables() const { return cables_; }

    // Podgląd kabla w trakcie ciągnięcia
    void setDragCable(juce::Point<float> from, juce::Point<float> to,
                      synth::SignalType type);
    void clearDragCable() { dragging_ = false; repaint(); }

    void paint(juce::Graphics& g) override;

private:
    void drawCable(juce::Graphics& g, juce::Point<float> a,
                   juce::Point<float> b, juce::Colour c, float thickness) const;

    std::vector<Cable> cables_;

    bool   dragging_ = false;
    juce::Point<float> dragFrom_, dragTo_;
    synth::SignalType  dragType_ = synth::SignalType::Audio;
};
