#include "SlotManager.h"
#include <stdexcept>

namespace synth {

SlotManager::SlotManager() = default;

ModuleGraph& SlotManager::slot(int index) {
    if (index < 0 || index >= kNumSlots)
        throw std::out_of_range("Slot index out of range");
    return slots_[index];
}

void SlotManager::setActiveSlot(int index) {
    if (index >= 0 && index < kNumSlots)
        activeSlot_ = index;
}

} // namespace synth
