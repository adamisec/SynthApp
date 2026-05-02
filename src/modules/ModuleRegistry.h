#pragma once
#include "engine/Module.h"
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>

namespace synth {

// ── Fabryka modułów ───────────────────────────────────────────────────────────
// Rejestruje wszystkie typy modułów pod nazwą string.
// Używana przez UI (ModuleBrowser) i parser patchy (.pch2 import).
class ModuleRegistry {
public:
    using Factory = std::function<std::unique_ptr<Module>()>;

    static ModuleRegistry& instance();

    void registerModule(const std::string& name, Factory factory);
    std::unique_ptr<Module> create(const std::string& name) const;

    // Lista wszystkich zarejestrowanych nazw (dla UI browser)
    std::vector<std::string> allNames() const;

private:
    ModuleRegistry();  // inicjalizuje wszystkie wbudowane moduły
    std::unordered_map<std::string, Factory> factories_;
};

} // namespace synth
