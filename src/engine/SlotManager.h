#pragma once
#include "ModuleGraph.h"
#include <array>
#include <string>

namespace synth {

// ── SlotManager — 4 niezależne sloty (jak w G2) ───────────────────────────────
// Każdy slot ma własny ModuleGraph i może grać niezależnie.
// Na razie szkielet — pełna implementacja w iteracji 4.
class SlotManager {
public:
    static constexpr int kNumSlots = 4;

    SlotManager();

    ModuleGraph& slot(int index);
    int          activeSlot() const { return activeSlot_; }
    void         setActiveSlot(int index);

    // Nazwy slotów (A/B/C/D) jak w G2
    static const char* slotName(int index) {
        static const char* names[] = { "A", "B", "C", "D" };
        return (index >= 0 && index < 4) ? names[index] : "?";
    }

private:
    std::array<ModuleGraph, kNumSlots> slots_;
    int activeSlot_ = 0;
};

} // namespace synth
