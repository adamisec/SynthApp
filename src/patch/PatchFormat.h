#pragma once
#include "engine/ModuleGraph.h"
#include <string>
#include <nlohmann/json.hpp>

namespace synth {

// ── Serializacja / deserializacja patchy ─────────────────────────────────────
// Format JSON kompatybilny z importem .pch2 (przez bridge Python)
struct PatchFormat {
    // Zapis grafu do JSON
    static nlohmann::json save(const ModuleGraph& graph,
                               const std::vector<Cable>& cables);

    // Odczyt z JSON i odbudowanie grafu
    static bool load(const nlohmann::json& j,
                     ModuleGraph& graph,
                     std::vector<Cable>& cables);

    // Pomocnicze: zapis/odczyt pliku
    static bool saveFile(const std::string& path, const ModuleGraph& graph,
                         const std::vector<Cable>& cables);
    static bool loadFile(const std::string& path, ModuleGraph& graph,
                         std::vector<Cable>& cables);
};

} // namespace synth
