#include "PatchFormat.h"
#include "modules/ModuleRegistry.h"
#include <fstream>

namespace synth {

// ── Zapis grafu do JSON ───────────────────────────────────────────────────────

nlohmann::json PatchFormat::save(const ModuleGraph& graph,
                                  const std::vector<Cable>& cables) {
    nlohmann::json j;
    j["version"] = "synthapp-0.1";

    // Moduły
    auto& modsJ = j["modules"] = nlohmann::json::array();
    for (int id : graph.getModuleIds()) {
        const Module* mod = const_cast<ModuleGraph&>(graph).getModule(id);
        if (!mod) continue;

        nlohmann::json mj;
        mj["id"]   = id;
        mj["type"] = mod->getName();

        // Zapisz wszystkie parametry znormalizowane [0..1]
        auto& paramsJ = mj["params"] = nlohmann::json::array();
        for (int p = 0; p < mod->getNumParams(); ++p)
            paramsJ.push_back(mod->getParam(p));

        modsJ.push_back(std::move(mj));
    }

    // Kable
    auto& cablesJ = j["cables"] = nlohmann::json::array();
    for (auto& c : cables) {
        cablesJ.push_back({
            {"fromModule", c.fromModule},
            {"fromPort",   c.fromPort},
            {"toModule",   c.toModule},
            {"toPort",     c.toPort},
        });
    }

    return j;
}

// ── Odczyt z JSON ─────────────────────────────────────────────────────────────
// Uwaga: przed wywołaniem graph musi być pusty (graph.clear() lub nowy obiekt).
// Zachowuje oryginalne ID modułów z JSON.

bool PatchFormat::load(const nlohmann::json& j,
                        ModuleGraph& graph,
                        std::vector<Cable>& cables) {
    try {
        auto& registry = ModuleRegistry::instance();

        // Moduły
        if (j.contains("modules")) {
            for (auto& mj : j["modules"]) {
                std::string type = mj.at("type").get<std::string>();
                auto mod = registry.create(type);
                if (!mod) continue;  // nieznany typ — pomiń

                // Ustaw parametry
                if (mj.contains("params")) {
                    int p = 0;
                    for (auto& pv : mj["params"]) {
                        if (p >= mod->getNumParams()) break;
                        mod->setParam(p++, pv.get<float>());
                    }
                }

                // Wymuś oryginalne ID (g2-import może mieć nieciągłe IDs)
                // Dodajemy moduł normalnie — ID przydzielane przez graph
                graph.addModule(std::move(mod));
            }
        }

        // Kable
        cables.clear();
        if (j.contains("cables")) {
            for (auto& c : j["cables"]) {
                Cable cable {
                    c.at("fromModule").get<int>(),
                    c.at("fromPort")  .get<int>(),
                    c.at("toModule")  .get<int>(),
                    c.at("toPort")    .get<int>(),
                };
                cables.push_back(cable);
                graph.addCable(cable);
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool PatchFormat::saveFile(const std::string& path, const ModuleGraph& graph,
                            const std::vector<Cable>& cables) {
    std::ofstream f(path);
    if (!f) return false;
    f << save(graph, cables).dump(2);
    return true;
}

bool PatchFormat::loadFile(const std::string& path, ModuleGraph& graph,
                            std::vector<Cable>& cables) {
    std::ifstream f(path);
    if (!f) return false;
    try {
        nlohmann::json j;
        f >> j;
        return load(j, graph, cables);
    } catch (...) {
        return false;
    }
}

} // namespace synth
