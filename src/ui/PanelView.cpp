#include "PanelView.h"

PanelView::PanelView(synth::ModuleGraph& graph) : graph_(graph) {
    addAndMakeVisible(slotTabs_);
    addAndMakeVisible(canvas_);
    addAndMakeVisible(cableOverlay_);

    slotTabs_.onSlotChanged = [this](int /*slot*/) {
        removeAllModuleViews();
        // TODO: załaduj moduły aktywnego slotu z grafu
    };

    setMouseClickGrabsKeyboardFocus(false);
}

PanelView::~PanelView() = default;

// ── Dodawanie modułów ────────────────────────────────────────────────────────

void PanelView::addModuleView(synth::Module& m, int moduleId, int x, int y) {
    auto* mv = new ModuleView(m, moduleId);
    mv->setTopLeftPosition(x, y);

    mv->onParamChanged = [this](int modId, int paramIdx, float val) {
        if (onParamChanged) onParamChanged(modId, paramIdx, val);
    };

    mv->onCableStart = [this](PortView* p) { startCableDrag(p); };

    // Przekaż callback do każdego PortView
    for (auto& pv : mv->ports()) {
        pv->onCableEnd = [this](PortView* p) { finishCableDrag(p); };
    }

    canvas_.addAndMakeVisible(mv);
    moduleViews_.emplace_back(mv);
    repaint();
}

void PanelView::addCableView(int fromModule, int fromPort,
                              int toModule, int toPort,
                              synth::SignalType type) {
    // Znajdź pozycje portów
    juce::Point<float> from, to;
    for (auto& mv : moduleViews_) {
        for (auto& pv : mv->ports()) {
            if (pv->moduleId() == fromModule && pv->portIndex() == fromPort) {
                from = pv->getBoundsInParent().toFloat().getCentre()
                     + mv->getPosition().toFloat();
                pv->setConnected(true);
            }
            if (pv->moduleId() == toModule && pv->portIndex() == toPort) {
                to = pv->getBoundsInParent().toFloat().getCentre()
                   + mv->getPosition().toFloat();
                pv->setConnected(true);
            }
        }
    }
    cableOverlay_.addCable({ from, to, type, fromModule, fromPort, toModule, toPort });
}

void PanelView::removeAllModuleViews() {
    canvas_.removeAllChildren();
    moduleViews_.clear();
    cableOverlay_.clearAllCables();
    repaint();
}

// ── Layout ────────────────────────────────────────────────────────────────────

void PanelView::resized() {
    slotTabs_.setBounds(0, 0, getWidth(), kSlotBarH);

    int canvasY = kSlotBarH;
    int canvasH = getHeight() - kSlotBarH - kStatusBarH;
    canvas_.setBounds(0, canvasY, getWidth(), canvasH);
    cableOverlay_.setBounds(canvas_.getBounds());
}

void PanelView::paint(juce::Graphics& g) {
    // Tło G2
    g.fillAll(juce::Colour(0xFF1A1A1A));

    // Pasek statusu
    g.setColour(juce::Colour(0xFF111111));
    g.fillRect(0, getHeight() - kStatusBarH, getWidth(), kStatusBarH);
    g.setColour(juce::Colour(0xFF555555));
    g.setFont(juce::FontOptions(10.0f));
    g.drawText("NORD MODULAR G2  — SynthApp",
               8, getHeight() - kStatusBarH, 300, kStatusBarH,
               juce::Justification::centredLeft, false);
}

// ── Zarządzanie kablami ───────────────────────────────────────────────────────

void PanelView::startCableDrag(PortView* port) {
    dragStartPort_ = port;
    auto pos = port->getBoundsInParent().toFloat().getCentre()
             + port->getParentComponent()->getPosition().toFloat();
    cableOverlay_.setDragCable(pos, pos, port->portInfo().type);
}

void PanelView::finishCableDrag(PortView* port) {
    if (!dragStartPort_ || port == dragStartPort_) {
        cancelCableDrag();
        return;
    }

    // Sprawdź zgodność kierunku: wyjście → wejście lub odwrotnie
    bool startIsOut = dragStartPort_->portInfo().isOutput;
    bool endIsOut   = port->portInfo().isOutput;
    if (startIsOut == endIsOut) { cancelCableDrag(); return; }

    PortView* from = startIsOut ? dragStartPort_ : port;
    PortView* to   = startIsOut ? port : dragStartPort_;

    addCableView(from->moduleId(), from->portIndex(),
                 to->moduleId(),   to->portIndex(),
                 from->portInfo().type);

    // TODO: dodaj połączenie do ModuleGraph
    // graph_.connect(from->moduleId(), from->portIndex(),
    //                to->moduleId(),   to->portIndex());

    dragStartPort_ = nullptr;
    cableOverlay_.clearDragCable();
}

void PanelView::cancelCableDrag() {
    dragStartPort_ = nullptr;
    cableOverlay_.clearDragCable();
}

void PanelView::mouseMove(const juce::MouseEvent& e) {
    if (dragStartPort_) {
        auto canvasPos = e.getEventRelativeTo(&canvas_).position;
        auto startPos  = dragStartPort_->getBoundsInParent().toFloat().getCentre()
                       + dragStartPort_->getParentComponent()->getPosition().toFloat();
        cableOverlay_.setDragCable(startPos, canvasPos,
                                   dragStartPort_->portInfo().type);
    }
}

void PanelView::mouseUp(const juce::MouseEvent& /*e*/) {
    if (dragStartPort_) cancelCableDrag();
}

void PanelView::mouseDown(const juce::MouseEvent& e) {
    if (e.mods.isRightButtonDown() && !dragStartPort_) {
        auto canvasPos = e.getEventRelativeTo(&canvas_).getPosition();
        showAddModuleMenu(canvasPos);
    }
}

void PanelView::showAddModuleMenu(juce::Point<int> canvasPos) {
    auto& reg = synth::ModuleRegistry::instance();
    auto names = reg.allNames();

    // Grupuj wg prefiksu nazwy
    struct Group { juce::String label; std::vector<juce::String> items; };
    std::vector<Group> groups = {
        { "Oscylatory",  {} },
        { "Filtry",      {} },
        { "Koperty",     {} },
        { "LFO",         {} },
        { "Miksery",     {} },
        { "Efekty",      {} },
        { "MIDI",        {} },
        { "Sekwencery",  {} },
        { "Inne",        {} },
    };
    auto groupFor = [](const juce::String& n) -> int {
        if (n.startsWith("Osc") || n == "Noise")  return 0;
        if (n.startsWith("Flt"))                   return 1;
        if (n.startsWith("Env"))                   return 2;
        if (n.startsWith("Lfo"))                   return 3;
        if (n == "VCA" || n.startsWith("Mix") || n == "Pan" || n == "Out") return 4;
        if (n.startsWith("Fx"))                    return 5;
        if (n == "KeyNote" || n == "KeyVelo" || n == "PitchBnd" || n == "ModWheel") return 6;
        if (n.startsWith("Clk") || n.startsWith("Seq")) return 7;
        return 8;
    };
    for (auto& name : names)
        groups[groupFor(name)].items.push_back(name);

    juce::PopupMenu menu;
    menu.addSectionHeader("Dodaj modul  (PPM)");
    menu.addSeparator();

    int itemId = 1;
    std::vector<juce::String> idToName;
    idToName.push_back(""); // index 0 unused

    for (auto& grp : groups) {
        if (grp.items.empty()) continue;
        juce::PopupMenu sub;
        for (auto& n : grp.items) {
            sub.addItem(itemId++, n);
            idToName.push_back(n);
        }
        menu.addSubMenu(grp.label, sub);
    }

    menu.showMenuAsync(juce::PopupMenu::Options{},
        [this, canvasPos, idToName = std::move(idToName)](int result) {
            if (result > 0 && result < (int)idToName.size())
                spawnModule(idToName[result], canvasPos);
        });
}

void PanelView::spawnModule(const juce::String& name, juce::Point<int> canvasPos) {
    auto mod = synth::ModuleRegistry::instance().create(name.toStdString());
    if (!mod) return;

    int id = graph_.addModule(std::move(mod));
    try { graph_.recompile(96000.0, 512); }
    catch (...) {}

    auto* m = graph_.getModule(id);
    if (m) addModuleView(*m, id, canvasPos.x, canvasPos.y);
}

void PanelView::removeModuleView(int moduleId) {
    for (auto it = moduleViews_.begin(); it != moduleViews_.end(); ++it) {
        if ((*it)->moduleId() == moduleId) {
            canvas_.removeChildComponent(it->get());
            moduleViews_.erase(it);
            break;
        }
    }
    graph_.removeModule(moduleId);
    try { graph_.recompile(96000.0, 512); }
    catch (...) {}
    cableOverlay_.clearAllCables();
    repaint();
}
