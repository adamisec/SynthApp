cmake_minimum_required(VERSION 3.25)
project(SynthApp VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── CPM (dependency manager) ──────────────────────────────────────────────────
include(cmake/CPM.cmake)

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    # Ubuntu 24.04 ma webkit2gtk-4.1, JUCE szuka 4.0.
    # Tworzymy lokalny alias .pc żeby pkg-config go znalazł.
    pkg_check_modules(WK41 webkit2gtk-4.1 QUIET)
    if(WK41_FOUND)
        set(FAKE_PC_DIR "${CMAKE_BINARY_DIR}/fake_pkgconfig")
        file(MAKE_DIRECTORY "${FAKE_PC_DIR}")
        file(WRITE "${FAKE_PC_DIR}/webkit2gtk-4.0.pc"
            "Name: webkit2gtk-4.0\nDescription: alias for 4.1\nVersion: 2.0\n"
            "Requires: webkit2gtk-4.1\nLibs:\nCflags:\n")
        file(WRITE "${FAKE_PC_DIR}/gtk+-x11-3.0.pc"
            "Name: gtk+-x11-3.0\nDescription: alias\nVersion: 3.0\n"
            "Requires: gtk+-3.0\nLibs:\nCflags:\n")
        set(ENV{PKG_CONFIG_PATH} "${FAKE_PC_DIR}:$ENV{PKG_CONFIG_PATH}")
    endif()
endif()

CPMAddPackage(
    NAME JUCE
    GIT_TAG 8.0.4
    GITHUB_REPOSITORY juce-framework/JUCE
)
CPMAddPackage(
    NAME nlohmann_json
    VERSION 3.11.3
    GITHUB_REPOSITORY nlohmann/json
    OPTIONS "JSON_BuildTests OFF"
)
CPMAddPackage(
    NAME Catch2
    VERSION 3.5.4
    GITHUB_REPOSITORY catchorg/Catch2
)

# ── Plugin target ─────────────────────────────────────────────────────────────
juce_add_plugin(SynthApp
    COMPANY_NAME            "SynthApp"
    PLUGIN_MANUFACTURER_CODE Snth
    PLUGIN_CODE             Snth
    FORMATS                 VST3 AU Standalone
    PRODUCT_NAME            "SynthApp G2"
    IS_SYNTH                TRUE
    NEEDS_MIDI_INPUT        TRUE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES         "Instrument" "Synth"
    AU_MAIN_TYPE            kAudioUnitType_MusicDevice
)

target_sources(SynthApp PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp

    # UI
    src/ui/KnobView.cpp
    src/ui/PortView.cpp
    src/ui/ModuleView.cpp
    src/ui/CableView.cpp
    src/ui/PanelView.cpp
    src/ui/SlotTabView.cpp
    src/ui/KeyboardView.cpp
    src/ui/SimplePanelView.cpp
    src/ui/PresetManager.cpp
    src/ui/SequencerView.cpp

    # engine
    src/engine/AudioEngine.cpp
    src/engine/Arpeggiator.cpp
    src/engine/Sequencer.cpp
    src/engine/ModuleGraph.cpp
    src/engine/VoicePool.cpp
    src/engine/SlotManager.cpp

    # modules
    src/modules/ModuleRegistry.cpp
    src/modules/oscillators/OscA.cpp
    src/modules/oscillators/OscB.cpp
    src/modules/oscillators/OscC.cpp
    src/modules/oscillators/OscD.cpp
    src/modules/oscillators/Noise.cpp
    src/modules/filters/FltLP.cpp
    src/modules/filters/FltNord.cpp
    src/modules/filters/FltVoice.cpp
    src/modules/filters/FltPhase.cpp
    src/modules/envelopes/EnvADSR.cpp
    src/modules/envelopes/EnvAD.cpp
    src/modules/lfos/LfoA.cpp
    src/modules/mixers/VCA.cpp
    src/modules/mixers/Mix4to1.cpp
    src/modules/mixers/Pan.cpp
    src/modules/mixers/Out.cpp
    src/modules/midi/KeyNote.cpp
    src/modules/midi/KeyVelo.cpp
    src/modules/midi/PitchBnd.cpp
    src/modules/midi/ModWheel.cpp
    src/modules/midi/KeyAfter.cpp
    src/modules/sequencers/ClkGen.cpp
    src/modules/sequencers/SeqNote.cpp
    src/modules/sequencers/SeqCtrl.cpp
    src/modules/sequencers/ClkDiv.cpp
    src/modules/lfos/LfoB.cpp
    src/modules/envelopes/EnvFollow.cpp
    src/modules/mixers/Mix8to1.cpp
    src/modules/effects/FxReverb.cpp
    src/modules/effects/FxDelay.cpp
    src/modules/effects/FxChorus.cpp
    src/modules/effects/FxDistort.cpp
    src/modules/effects/FxCompressor.cpp
    src/modules/effects/FxEQ.cpp

    # patch
    src/patch/PatchFormat.cpp
    src/patch/CableSystem.cpp
)

target_include_directories(SynthApp PRIVATE src)

target_compile_definitions(SynthApp PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(SynthApp PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_basics
    nlohmann_json::nlohmann_json
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
    asound
)

# ── Tests ─────────────────────────────────────────────────────────────────────
add_executable(SynthAppTests
    tests/test_osc_aliasing.cpp
    tests/test_filter_response.cpp
    tests/test_envelope_timing.cpp
    tests/test_graph_topo.cpp
    tests/test_effects.cpp
    tests/test_sequencers.cpp
    tests/test_patch_format.cpp

    # źródła silnika (bez JUCE GUI) linkowane do testów
    src/engine/ModuleGraph.cpp
    src/engine/VoicePool.cpp
    src/modules/ModuleRegistry.cpp
    src/modules/oscillators/OscA.cpp
    src/modules/oscillators/OscB.cpp
    src/modules/oscillators/OscC.cpp
    src/modules/oscillators/OscD.cpp
    src/modules/oscillators/Noise.cpp
    src/modules/filters/FltLP.cpp
    src/modules/filters/FltNord.cpp
    src/modules/filters/FltVoice.cpp
    src/modules/filters/FltPhase.cpp
    src/modules/envelopes/EnvADSR.cpp
    src/modules/envelopes/EnvAD.cpp
    src/modules/lfos/LfoA.cpp
    src/modules/mixers/VCA.cpp
    src/modules/mixers/Mix4to1.cpp
    src/modules/mixers/Pan.cpp
    src/modules/mixers/Out.cpp
    src/modules/midi/KeyNote.cpp
    src/modules/midi/KeyVelo.cpp
    src/modules/midi/PitchBnd.cpp
    src/modules/midi/ModWheel.cpp
    src/modules/midi/KeyAfter.cpp
    src/modules/sequencers/ClkGen.cpp
    src/modules/sequencers/SeqNote.cpp
    src/modules/sequencers/SeqCtrl.cpp
    src/modules/sequencers/ClkDiv.cpp
    src/modules/lfos/LfoB.cpp
    src/modules/envelopes/EnvFollow.cpp
    src/modules/mixers/Mix8to1.cpp
    src/modules/effects/FxReverb.cpp
    src/modules/effects/FxDelay.cpp
    src/modules/effects/FxChorus.cpp
    src/modules/effects/FxDistort.cpp
    src/modules/effects/FxCompressor.cpp
    src/modules/effects/FxEQ.cpp
    src/patch/PatchFormat.cpp
    src/patch/CableSystem.cpp
)

target_include_directories(SynthAppTests PRIVATE src)
target_link_libraries(SynthAppTests PRIVATE Catch2::Catch2WithMain nlohmann_json::nlohmann_json)

include(CTest)
list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(SynthAppTests)
