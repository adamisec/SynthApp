#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <nlohmann/json.hpp>

// ── PresetManager — zapis/odczyt presetów syntezatora ────────────────────────
// Presety przechowywane jako JSON w katalogu uzytkownika.
// Fabryczne presety wbudowane na stałe.
class PresetManager {
public:
    struct ParamValue {
        int   moduleId;
        int   paramIdx;
        float value;
    };

    // Krok sekwencera — nuta MIDI + aktywność
    struct SeqStep {
        int  note   = 60;
        bool active = true;
    };

    struct Preset {
        juce::String            name;
        bool                    isFactory  = false;
        std::vector<ParamValue> params;

        // Opcjonalne dane sekwencera
        bool                   hasSeq    = false;
        float                  seqBPM    = 120.0f;
        int                    seqDiv    = 16;     // 8 lub 16
        int                    seqSteps  = 8;
        std::vector<SeqStep>   seqPattern;
    };

    PresetManager();

    // Lista nazw wszystkich presetów (fabr. + uzytkownika)
    juce::StringArray  getNames() const;
    int                getCount() const { return (int)presets_.size(); }

    // Zwraca preset o danym indeksie (z listy getNames)
    const Preset*      getPreset(int index) const;
    const Preset*      getPreset(const juce::String& name) const;

    // Zapis presetu uzytkownika (nadpisuje jeśli istnieje)
    void               savePreset(const juce::String& name,
                                  const std::vector<ParamValue>& params);

    // Usuwa preset uzytkownika (fabryczne nieusuwalne)
    bool               deletePreset(const juce::String& name);

    // Plik uzytkownika
    juce::File         userFile() const;
    void               saveToFile();
    void               loadFromFile();

private:
    void               buildFactoryPresets();

    std::vector<Preset> presets_;   // fabr. na początku, uzytkownika po nich
};
