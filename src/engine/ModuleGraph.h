#pragma once
#include "Module.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <atomic>
#include <array>
#include <functional>

namespace synth {

// ── Opis połączenia (kabla) ───────────────────────────────────────────────────
struct Cable {
    int fromModule;  // ID modułu źródłowego
    int fromPort;    // indeks portu wyjściowego (w kolejności portów output)
    int toModule;    // ID modułu docelowego
    int toPort;      // indeks portu wejściowego (w kolejności portów input)
};

// ── Skompilowany plan przetwarzania ──────────────────────────────────────────
// Powstaje po każdym addModule/connect/disconnect (w message thread).
// Swap atomowy z audio thread — zero locków w RT path.
struct ProcessingPlan {
    struct Step {
        Module*                   module;
        std::vector<SignalBuffer*> inputBuffers;   // nullptr = port niepodłączony
        std::vector<SignalBuffer*> outputBuffers;
    };

    std::vector<Step>                          steps;
    std::vector<std::unique_ptr<SignalBuffer>> bufferPool;  // własność planu
};

// ── Graf modułów ─────────────────────────────────────────────────────────────
class ModuleGraph {
public:
    ModuleGraph();
    ~ModuleGraph();

    // ── Modyfikacja grafu (message thread) ───────────────────────────────────
    // Dodaje moduł, zwraca jego ID
    int  addModule(std::unique_ptr<Module> module);
    void removeModule(int moduleId);

    void addCable(Cable cable);
    void removeCable(Cable cable);

    // Kompiluje kolejność przetwarzania po zmianach topologii
    // Rzuca std::runtime_error przy wykryciu cyklu
    void recompile(double sampleRate, int blockSize);

    // ── Przetwarzanie (audio thread) ─────────────────────────────────────────
    // Bezpieczne do wywołania bez locków — używa atomowego swapa planów
    void process(int numSamples);

    // Callback błędu topologii (np. cykl) — wywoływany z message thread
    std::function<void(const std::string&)> onError;

    // Dostęp do modułu po ID (message thread only)
    Module* getModule(int id);

    // Iteracja modułów i kabli (message thread, do serializacji)
    std::vector<int>   getModuleIds()  const;
    const std::vector<Cable>& getCables() const { return cables_; }

    // Odczyt bufora wyjściowego modułu po ID i indeksie wyjścia (audio thread,
    // wywołać po process()). Zwraca nullptr jeśli moduł/port nie istnieje.
    const float* getOutputBuffer(int moduleId, int outPortIdx) const;

private:
    // Kahn's topological sort, zwraca posortowane ID lub pusty wektor przy cyklu
    std::vector<int> topoSort(const std::vector<int>& moduleIds,
                              const std::vector<Cable>& cables);

    std::unique_ptr<ProcessingPlan> buildPlan(
        const std::vector<int>& order,
        double sampleRate,
        int blockSize);

    // Dane modyfikowane z message thread
    std::unordered_map<int, std::unique_ptr<Module>> modules_;
    std::vector<Cable>                               cables_;
    int                                              nextId_ = 0;

    // Double-buffer planu przetwarzania
    // Slot [0] lub [1] jest aktywny — audio thread czyta, message thread pisze
    std::array<std::unique_ptr<ProcessingPlan>, 2> plans_;
    std::atomic<int>                                activePlan_{ 0 };
};

} // namespace synth
