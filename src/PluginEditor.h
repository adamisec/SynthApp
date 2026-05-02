#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#if JUCE_STANDALONE_APPLICATION
 #include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif
#include "ui/SimplePanelView.h"
#include "ui/KeyboardView.h"
#include "ui/PresetManager.h"
#include "ui/SequencerView.h"
#include "engine/Arpeggiator.h"
#include <map>
#include <unordered_map>

namespace synth { class AudioEngine; }

// ── PluginEditor — glowne okno UI ────────────────────────────────────────────
class PluginEditor : public juce::AudioProcessorEditor,
                     private juce::Timer {
public:
    explicit PluginEditor(synth::AudioEngine& engine);
    ~PluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // Klawiatura komputerowa — działa bo PluginEditor trzyma focus
    bool keyPressed    (const juce::KeyPress& key) override;
    bool keyStateChanged(bool isKeyDown)            override;

private:
    void timerCallback() override;
    void openAudioSettings();
    void loadPreset(int index);
    void saveCurrentPreset();
    void updateOctaveLabel();

    // Zwraca nute MIDI dla danego kodu klawisza (bez przesuniecia oktawy),
    // lub -1 jesli klawisz nie jest zmapowany.
    static int keyCodeToMidi(int keyCode);

    synth::AudioEngine& engine_;
    SimplePanelView     panel_;
    KeyboardView        keyboard_;
    SequencerView       seqView_;
    PresetManager       presets_;

    // ── Pasek naglowka ───────────────────────────────────────────────────────
    juce::Label      titleLabel_;
    juce::ComboBox   presetBox_;
    juce::TextButton prevBtn_    { "<" };
    juce::TextButton nextBtn_    { ">" };
    juce::TextButton saveBtn_    { "SAVE" };
    juce::TextButton deleteBtn_  { "DEL" };
    juce::TextButton octDownBtn_ { "Oct-" };
    juce::TextButton octUpBtn_   { "Oct+" };
    juce::Label      octLabel_;
    juce::TextButton audioBtn_   { "AUDIO" };
    juce::Label      perfLabel_;

    int currentPreset_  = 0;
    int octaveShift_    = 0;   // -3..+3 oktaw

    // keyCode → MIDI note aktualnie trzymanych klawiszy
    std::map<int, int> heldKeys_;

    // ── Pasek arpeggiatora ────────────────────────────────────────────────────
    juce::TextButton arpOnBtn_  { "ARP" };
    juce::TextButton arpUpBtn_  { "UP \xe2\x96\xb2" };
    juce::TextButton arpDnBtn_  { "DN \xe2\x96\xbc" };
    juce::TextButton arpUdBtn_  { "U/D" };
    juce::TextButton arpRndBtn_ { "RND" };

    juce::Label      bpmLabel_;
    juce::TextButton bpmDnBtn_  { "-" };
    juce::TextButton bpmUpBtn_  { "+" };

    juce::TextButton rate4Btn_  { "1/4"  };
    juce::TextButton rate8Btn_  { "1/8"  };
    juce::TextButton rate16Btn_ { "1/16" };
    juce::TextButton rate32Btn_ { "1/32" };

    juce::TextButton oct1Btn_   { "1" };
    juce::TextButton oct2Btn_   { "2" };
    juce::TextButton oct3Btn_   { "3" };

    float arpBPM_  = 120.0f;
    int   arpRate_ = 16;
    int   arpOct_  = 1;
    synth::Arpeggiator::Pattern arpPattern_ = synth::Arpeggiator::Pattern::Up;

    void updateBpmLabel();
    void selectArpPattern(synth::Arpeggiator::Pattern p);
    void selectArpRate   (int division);
    void selectArpOct    (int oct);

    static constexpr int kHeaderH   = 44;
    static constexpr int kSeqH      = 110;
    static constexpr int kArpH      = 40;
    static constexpr int kKeyboardH = 80;
};
