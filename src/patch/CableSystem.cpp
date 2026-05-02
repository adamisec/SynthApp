#include "CableSystem.h"

namespace synth {

CableSystem::CableSystem(ModuleGraph& graph) : graph_(graph) {}

bool CableSystem::connect(int fromModule, int fromPort,
                           int toModule,   int toPort,
                           double sampleRate, int blockSize) {
    // Pobierz typy portów
    Module* src = graph_.getModule(fromModule);
    Module* dst = graph_.getModule(toModule);
    if (!src || !dst) return false;

    auto srcPorts = src->getPorts();
    auto dstPorts = dst->getPorts();

    // Znajdź n-ty port wyjściowy źródła
    int outIdx = 0;
    SignalType srcType = SignalType::Audio;
    for (auto& p : srcPorts) {
        if (p.isOutput) {
            if (outIdx == fromPort) { srcType = p.type; break; }
            outIdx++;
        }
    }

    // Znajdź n-ty port wejściowy celu
    int inIdx = 0;
    SignalType dstType = SignalType::Audio;
    for (auto& p : dstPorts) {
        if (!p.isOutput) {
            if (inIdx == toPort) { dstType = p.type; break; }
            inIdx++;
        }
    }

    // Typy muszą pasować (audio→audio, cv→cv, gate→gate)
    if (srcType != dstType) return false;

    Cable c { fromModule, fromPort, toModule, toPort };
    cables_.push_back({ c, srcType });
    graph_.addCable(c);
    graph_.recompile(sampleRate, blockSize);
    return true;
}

void CableSystem::disconnect(int fromModule, int fromPort,
                              int toModule,   int toPort,
                              double sampleRate, int blockSize) {
    Cable c { fromModule, fromPort, toModule, toPort };
    cables_.erase(
        std::remove_if(cables_.begin(), cables_.end(),
            [&](const CableDescriptor& cd) {
                return cd.cable.fromModule == c.fromModule &&
                       cd.cable.fromPort   == c.fromPort   &&
                       cd.cable.toModule   == c.toModule   &&
                       cd.cable.toPort     == c.toPort;
            }),
        cables_.end()
    );
    graph_.removeCable(c);
    graph_.recompile(sampleRate, blockSize);
}

} // namespace synth
