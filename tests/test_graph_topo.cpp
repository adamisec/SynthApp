#include <catch2/catch_test_macros.hpp>
#include "engine/ModuleGraph.h"
#include "modules/oscillators/OscA.h"
#include "modules/filters/FltLP.h"
#include "modules/envelopes/EnvADSR.h"
#include "modules/mixers/VCA.h"
#include "modules/midi/KeyNote.h"

using namespace synth;

TEST_CASE("ModuleGraph: prosty łańcuch kompiluje bez błędu") {
    ModuleGraph graph;

    int keyId  = graph.addModule(std::make_unique<KeyNote>());
    int oscId  = graph.addModule(std::make_unique<OscA>());
    int fltId  = graph.addModule(std::make_unique<FltLP>());
    int vcaId  = graph.addModule(std::make_unique<VCA>());
    int envId  = graph.addModule(std::make_unique<EnvADSR>());

    // KeyNote.PitchCV → OscA.PitchCV
    graph.addCable({ keyId, 0, oscId, 0 });
    // OscA.AudioOut → FltLP.AudioIn
    graph.addCable({ oscId, 0, fltId, 0 });
    // FltLP.AudioOut → VCA.AudioIn
    graph.addCable({ fltId, 0, vcaId, 0 });
    // KeyNote.Gate → EnvADSR.Gate
    graph.addCable({ keyId, 1, envId, 0 });
    // EnvADSR.EnvOut → VCA.CVGain
    graph.addCable({ envId, 0, vcaId, 1 });

    bool errorFired = false;
    graph.onError = [&](const std::string&) { errorFired = true; };

    REQUIRE_NOTHROW(graph.recompile(96000.0, 64));
    REQUIRE_FALSE(errorFired);
}

TEST_CASE("ModuleGraph: cykl wykrywany i raportowany") {
    ModuleGraph graph;

    int a = graph.addModule(std::make_unique<OscA>());
    int b = graph.addModule(std::make_unique<FltLP>());

    graph.addCable({ a, 0, b, 0 });
    graph.addCable({ b, 0, a, 0 });  // cykl: a→b→a

    bool errorFired = false;
    graph.onError = [&](const std::string& msg) {
        errorFired = true;
        REQUIRE_FALSE(msg.empty());
    };

    graph.recompile(96000.0, 64);
    REQUIRE(errorFired);
}

TEST_CASE("ModuleGraph: usunięcie modułu usuwa też jego kable") {
    ModuleGraph graph;

    int oscId = graph.addModule(std::make_unique<OscA>());
    int fltId = graph.addModule(std::make_unique<FltLP>());

    graph.addCable({ oscId, 0, fltId, 0 });
    graph.removeModule(oscId);

    bool errorFired = false;
    graph.onError = [&](const std::string&) { errorFired = true; };

    REQUIRE_NOTHROW(graph.recompile(96000.0, 64));
    REQUIRE_FALSE(errorFired);
}

TEST_CASE("ModuleGraph: process nie crashuje na pustym grafie") {
    ModuleGraph graph;
    graph.recompile(96000.0, 64);
    REQUIRE_NOTHROW(graph.process(64));
}

TEST_CASE("ModuleGraph: process nie crashuje na kompletnym patchu") {
    ModuleGraph graph;

    int keyId = graph.addModule(std::make_unique<KeyNote>());
    int oscId = graph.addModule(std::make_unique<OscA>());
    int fltId = graph.addModule(std::make_unique<FltLP>());
    int envId = graph.addModule(std::make_unique<EnvADSR>());
    int vcaId = graph.addModule(std::make_unique<VCA>());

    graph.addCable({ keyId, 0, oscId, 0 });
    graph.addCable({ oscId, 0, fltId, 0 });
    graph.addCable({ fltId, 0, vcaId, 0 });
    graph.addCable({ keyId, 1, envId, 0 });
    graph.addCable({ envId, 0, vcaId, 1 });

    // Ustaw nutę A4
    graph.getModule(keyId)->setParam(KeyNote::kNote,     69.0f / 127.0f);
    graph.getModule(keyId)->setParam(KeyNote::kVelocity, 0.8f);
    graph.getModule(keyId)->setParam(KeyNote::kGate,     1.0f);

    graph.recompile(96000.0, 64);

    // Kilka bloków bez crashy
    for (int b = 0; b < 10; ++b)
        REQUIRE_NOTHROW(graph.process(64));
}
