#pragma once
#include "engine/ModuleGraph.h"
#include <vector>
#include <optional>

namespace synth {

// ── CableSystem — zarządza kablami i synchronizuje z ModuleGraph ─────────────
// Warstwa pośrednia między UI a ModuleGraph:
//   - Waliduje połączenia (typy portów muszą pasować)
//   - Przechowuje metadane wizualne (kolor kabla)
//   - Powiadamia o zmianach → ModuleGraph.recompile()
struct CableDescriptor {
    Cable       cable;
    SignalType  signalType;   // determinuje kolor w UI
};

class CableSystem {
public:
    explicit CableSystem(ModuleGraph& graph);

    // Próba dodania kabla — zwraca false jeśli typy portów nie pasują
    bool connect(int fromModule, int fromPort,
                 int toModule,   int toPort,
                 double sampleRate, int blockSize);

    void disconnect(int fromModule, int fromPort,
                    int toModule,   int toPort,
                    double sampleRate, int blockSize);

    const std::vector<CableDescriptor>& cables() const { return cables_; }

private:
    ModuleGraph&               graph_;
    std::vector<CableDescriptor> cables_;
};

} // namespace synth
