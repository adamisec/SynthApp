#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "engine/Module.h"
#include <functional>

// ── PortView — gniazdo kabla (port modułu) ────────────────────────────────────
// Kliknięcie rozpoczyna/kończy ciągnięcie kabla.
// Kolor: Audio=żółty, CV=niebieski, Gate=pomarańczowy
class PortView : public juce::Component {
public:
    PortView(const synth::PortInfo& info, int moduleId, int portIndex);

    // Callbacks dla zarządzania kablami (ustawiane przez PanelView)
    std::function<void(PortView*)>  onCableStart;   // kliknięto port
    std::function<void(PortView*)>  onCableEnd;     // zakończono ciągnięcie

    const synth::PortInfo& portInfo()  const { return info_; }
    int moduleId()   const { return moduleId_; }
    int portIndex()  const { return portIndex_; }

    bool isConnected() const { return connected_; }
    void setConnected(bool c) { connected_ = c; repaint(); }

    // Pozycja środka portu w układzie rodzica
    juce::Point<float> centrePosInParent() const;

    static juce::Colour colourFor(synth::SignalType t);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;

private:
    synth::PortInfo info_;
    int  moduleId_  = -1;
    int  portIndex_ = -1;
    bool connected_ = false;
};
