#include "ModuleGraph.h"
#include <stdexcept>
#include <algorithm>
#include <cassert>
#include <unordered_set>
#include <queue>

namespace synth {

ModuleGraph::ModuleGraph() {
    plans_[0] = std::make_unique<ProcessingPlan>();
    plans_[1] = std::make_unique<ProcessingPlan>();
}

ModuleGraph::~ModuleGraph() = default;

// ── Modyfikacja grafu ─────────────────────────────────────────────────────────

int ModuleGraph::addModule(std::unique_ptr<Module> module) {
    int id = nextId_++;
    module->id = id;
    modules_[id] = std::move(module);
    return id;
}

void ModuleGraph::removeModule(int moduleId) {
    modules_.erase(moduleId);
    // usuń wszystkie kable związane z tym modułem
    cables_.erase(
        std::remove_if(cables_.begin(), cables_.end(), [moduleId](const Cable& c) {
            return c.fromModule == moduleId || c.toModule == moduleId;
        }),
        cables_.end()
    );
}

void ModuleGraph::addCable(Cable cable) {
    cables_.push_back(cable);
}

void ModuleGraph::removeCable(Cable cable) {
    cables_.erase(
        std::remove_if(cables_.begin(), cables_.end(), [&](const Cable& c) {
            return c.fromModule == cable.fromModule &&
                   c.fromPort   == cable.fromPort   &&
                   c.toModule   == cable.toModule   &&
                   c.toPort     == cable.toPort;
        }),
        cables_.end()
    );
}

// ── Kompilacja planu ──────────────────────────────────────────────────────────

void ModuleGraph::recompile(double sampleRate, int blockSize) {
    // Zbierz ID wszystkich modułów
    std::vector<int> ids;
    ids.reserve(modules_.size());
    for (auto& [id, _] : modules_) ids.push_back(id);

    auto order = topoSort(ids, cables_);

    if (order.empty() && !ids.empty()) {
        if (onError) onError("Wykryto cykl w grafie modułów.");
        return;
    }

    auto newPlan = buildPlan(order, sampleRate, blockSize);

    // Swap: wstaw nowy plan do nieaktywnego slotu
    int inactive = 1 - activePlan_.load(std::memory_order_acquire);
    plans_[inactive] = std::move(newPlan);
    activePlan_.store(inactive, std::memory_order_release);
}

// ── Przetwarzanie (audio thread) ──────────────────────────────────────────────

void ModuleGraph::process(int numSamples) {
    int active = activePlan_.load(std::memory_order_acquire);
    ProcessingPlan& plan = *plans_[active];

    for (auto& step : plan.steps) {
        // Zbierz wskaźniki wejść/wyjść
        std::vector<const float*> ins;
        std::vector<float*>       outs;

        for (auto* buf : step.inputBuffers)
            ins.push_back(buf ? buf->ptr() : nullptr);

        for (auto* buf : step.outputBuffers) {
            buf->clear(numSamples);
            outs.push_back(buf->ptr());
        }

        step.module->process(ins.data(), outs.data(), numSamples);
    }
}

// ── Dostęp do modułu ─────────────────────────────────────────────────────────

std::vector<int> ModuleGraph::getModuleIds() const {
    std::vector<int> ids;
    ids.reserve(modules_.size());
    for (auto& [id, _] : modules_) ids.push_back(id);
    std::sort(ids.begin(), ids.end());
    return ids;
}

Module* ModuleGraph::getModule(int id) {
    auto it = modules_.find(id);
    return it != modules_.end() ? it->second.get() : nullptr;
}

const float* ModuleGraph::getOutputBuffer(int moduleId, int outPortIdx) const {
    int active = activePlan_.load(std::memory_order_acquire);
    const ProcessingPlan& plan = *plans_[active];
    for (const auto& step : plan.steps) {
        if (step.module->id == moduleId) {
            if (outPortIdx < (int)step.outputBuffers.size() &&
                step.outputBuffers[outPortIdx] != nullptr)
                return step.outputBuffers[outPortIdx]->ptr();
            return nullptr;
        }
    }
    return nullptr;
}

// ── Kahn's topological sort ───────────────────────────────────────────────────

std::vector<int> ModuleGraph::topoSort(const std::vector<int>& ids,
                                        const std::vector<Cable>& cables) {
    // Buduj graf sąsiedztwa i in-degree
    std::unordered_map<int, std::vector<int>> adj;   // from → {to}
    std::unordered_map<int, int>              inDeg;

    for (int id : ids) {
        adj[id];      // upewnij się, że klucz istnieje
        inDeg[id] = 0;
    }
    for (auto& c : cables) {
        if (adj.count(c.fromModule) && inDeg.count(c.toModule)) {
            adj[c.fromModule].push_back(c.toModule);
            inDeg[c.toModule]++;
        }
    }

    // Kahn: zacznij od węzłów bez wejść
    std::queue<int> q;
    for (auto& [id, deg] : inDeg)
        if (deg == 0) q.push(id);

    std::vector<int> order;
    order.reserve(ids.size());

    while (!q.empty()) {
        int cur = q.front(); q.pop();
        order.push_back(cur);
        for (int next : adj[cur]) {
            if (--inDeg[next] == 0)
                q.push(next);
        }
    }

    // Jeśli nie wszystkie węzły odwiedzone → cykl
    if (order.size() != ids.size())
        return {};

    return order;
}

// ── Budowanie planu przetwarzania ─────────────────────────────────────────────

std::unique_ptr<ProcessingPlan> ModuleGraph::buildPlan(
    const std::vector<int>& order,
    double sampleRate,
    int blockSize)
{
    auto plan = std::make_unique<ProcessingPlan>();

    // Mapa: (moduleId, portIndex_output) → bufor
    std::unordered_map<int, std::unordered_map<int, SignalBuffer*>> outputBuffers;

    for (int moduleId : order) {
        Module* mod = modules_[moduleId].get();
        mod->prepare(sampleRate, blockSize);

        auto ports = mod->getPorts();

        // Policz oddzielnie porty wejściowe i wyjściowe
        int numIn  = 0, numOut = 0;
        for (auto& p : ports) p.isOutput ? numOut++ : numIn++;

        ProcessingPlan::Step step;
        step.module = mod;
        step.inputBuffers.resize(numIn,  nullptr);
        step.outputBuffers.resize(numOut, nullptr);

        // Podłącz bufory wyjściowe (utwórz nowe)
        int outIdx = 0;
        for (auto& p : ports) {
            if (!p.isOutput) continue;
            auto buf = std::make_unique<SignalBuffer>();
            step.outputBuffers[outIdx] = buf.get();
            outputBuffers[moduleId][outIdx] = buf.get();
            plan->bufferPool.push_back(std::move(buf));
            outIdx++;
        }

        // Podłącz bufory wejściowe (z wyjść wcześniejszych modułów)
        int inIdx = 0;
        for (auto& p : ports) {
            if (p.isOutput) continue;
            // Szukaj kabla podłączonego do tego wejścia
            for (auto& c : cables_) {
                if (c.toModule == moduleId && c.toPort == inIdx) {
                    auto it = outputBuffers.find(c.fromModule);
                    if (it != outputBuffers.end()) {
                        auto it2 = it->second.find(c.fromPort);
                        if (it2 != it->second.end())
                            step.inputBuffers[inIdx] = it2->second;
                    }
                    break;
                }
            }
            inIdx++;
        }

        plan->steps.push_back(std::move(step));
    }

    return plan;
}

} // namespace synth
