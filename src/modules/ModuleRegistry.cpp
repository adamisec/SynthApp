#include "ModuleRegistry.h"
#include "oscillators/OscA.h"
#include "oscillators/OscB.h"
#include "oscillators/OscC.h"
#include "oscillators/OscD.h"
#include "oscillators/Noise.h"
#include "filters/FltLP.h"
#include "filters/FltNord.h"
#include "filters/FltVoice.h"
#include "filters/FltPhase.h"
#include "envelopes/EnvADSR.h"
#include "envelopes/EnvAD.h"
#include "lfos/LfoA.h"
#include "mixers/VCA.h"
#include "mixers/Mix4to1.h"
#include "mixers/Pan.h"
#include "mixers/Out.h"
#include "midi/KeyNote.h"
#include "midi/KeyVelo.h"
#include "midi/PitchBnd.h"
#include "midi/ModWheel.h"
#include "midi/KeyAfter.h"
#include "effects/FxEQ.h"
#include "sequencers/ClkGen.h"
#include "sequencers/SeqNote.h"
#include "sequencers/SeqCtrl.h"
#include "sequencers/ClkDiv.h"
#include "lfos/LfoB.h"
#include "envelopes/EnvFollow.h"
#include "mixers/Mix8to1.h"
#include "effects/FxReverb.h"
#include "effects/FxDelay.h"
#include "effects/FxChorus.h"
#include "effects/FxDistort.h"
#include "effects/FxCompressor.h"

namespace synth {

ModuleRegistry& ModuleRegistry::instance() {
    static ModuleRegistry inst;
    return inst;
}

ModuleRegistry::ModuleRegistry() {
    // Oscylatory
    registerModule("OscA",    []{ return std::make_unique<OscA>(); });
    registerModule("OscB",    []{ return std::make_unique<OscB>(); });
    registerModule("OscC",    []{ return std::make_unique<OscC>(); });
    registerModule("OscD",    []{ return std::make_unique<OscD>(); });
    registerModule("Noise",   []{ return std::make_unique<Noise>(); });

    // Filtry
    registerModule("FltLP",    []{ return std::make_unique<FltLP>(); });
    registerModule("FltNord",  []{ return std::make_unique<FltNord>(); });
    registerModule("FltVoice", []{ return std::make_unique<FltVoice>(); });
    registerModule("FltPhase", []{ return std::make_unique<FltPhase>(); });

    // Koperty
    registerModule("EnvADSR", []{ return std::make_unique<EnvADSR>(); });
    registerModule("EnvAD",   []{ return std::make_unique<EnvAD>(); });

    // LFO
    registerModule("LfoA",    []{ return std::make_unique<LfoA>(); });

    // Miksery / Out
    registerModule("VCA",     []{ return std::make_unique<VCA>(); });
    registerModule("Mix4to1", []{ return std::make_unique<Mix4to1>(); });
    registerModule("Pan",     []{ return std::make_unique<Pan>(); });
    registerModule("Out",     []{ return std::make_unique<Out>(); });

    // MIDI
    registerModule("KeyNote",  []{ return std::make_unique<KeyNote>(); });
    registerModule("KeyVelo",  []{ return std::make_unique<KeyVelo>(); });
    registerModule("PitchBnd", []{ return std::make_unique<PitchBnd>(); });
    registerModule("ModWheel", []{ return std::make_unique<ModWheel>(); });
    registerModule("KeyAfter", []{ return std::make_unique<KeyAfter>(); });

    // LFO
    registerModule("LfoB",     []{ return std::make_unique<LfoB>(); });

    // Koperty
    registerModule("EnvFollow",[]{ return std::make_unique<EnvFollow>(); });

    // Miksery
    registerModule("Mix8to1",  []{ return std::make_unique<Mix8to1>(); });

    // Sekwencer / Zegar
    registerModule("ClkGen",   []{ return std::make_unique<ClkGen>(); });
    registerModule("SeqNote",  []{ return std::make_unique<SeqNote>(); });
    registerModule("SeqCtrl",  []{ return std::make_unique<SeqCtrl>(); });
    registerModule("ClkDiv",   []{ return std::make_unique<ClkDiv>(); });

    // Efekty
    registerModule("FxReverb",     []{ return std::make_unique<FxReverb>(); });
    registerModule("FxDelay",      []{ return std::make_unique<FxDelay>(); });
    registerModule("FxChorus",     []{ return std::make_unique<FxChorus>(); });
    registerModule("FxDistort",    []{ return std::make_unique<FxDistort>(); });
    registerModule("FxCompressor", []{ return std::make_unique<FxCompressor>(); });
    registerModule("FxEQ",         []{ return std::make_unique<FxEQ>(); });
}

void ModuleRegistry::registerModule(const std::string& name, Factory factory) {
    factories_[name] = std::move(factory);
}

std::unique_ptr<Module> ModuleRegistry::create(const std::string& name) const {
    auto it = factories_.find(name);
    if (it == factories_.end()) return nullptr;
    return it->second();
}

std::vector<std::string> ModuleRegistry::allNames() const {
    std::vector<std::string> names;
    names.reserve(factories_.size());
    for (auto& [k, _] : factories_) names.push_back(k);
    std::sort(names.begin(), names.end());
    return names;
}

} // namespace synth
