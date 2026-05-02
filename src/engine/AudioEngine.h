#pragma once
#include "VoicePool.h"
#include "NoteEvent.h"
#include "Arpeggiator.h"
#include "Sequencer.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

namespace synth {

// ── AudioEngine — główny AudioProcessor JUCE ─────────────────────────────────
// Łączy MIDI → VoicePool → audio output.
// Jeden slot (rozszerzalne do 4 slotów w następnej iteracji).
class AudioEngine : public juce::AudioProcessor {
public:
    AudioEngine();
    ~AudioEngine() override = default;

    // ── AudioProcessor interface ──────────────────────────────────────────────
    const juce::String getName() const override { return "SynthApp G2"; }

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    bool acceptsMidi()  const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    bool hasEditor()    const override { return true; }

    double getTailLengthSeconds() const override { return 2.0; }

    int  getNumPrograms()                        override { return 1; }
    int  getCurrentProgram()                     override { return 0; }
    void setCurrentProgram(int)                  override {}
    const juce::String getProgramName(int)       override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&)        override {}
    void setStateInformation(const void*, int)           override {}

    juce::AudioProcessorEditor* createEditor() override;

    // ── Dostęp dla UI ─────────────────────────────────────────────────────────
    int          activeVoices() const { return voicePool_.activeVoiceCount(); }
    // Graf głosu 0 — tylko do odczytu w UI (nie modyfikować w audio thread)
    ModuleGraph& displayGraph() { return voicePool_.voiceGraph(0); }

    // Wyzwalanie nut z wirtualnej klawiatury (thread-safe przez MidiMessageCollector)
    void noteOn (int midiNote, float velocity);
    void noteOff(int midiNote);

    // Zdarzenia globalne (thread-safe)
    void pitchBend  (float v01) { voicePool_.setPitchBend(v01); }
    void modWheel   (float v01) { voicePool_.setModWheel(v01); }
    void aftertouch (float v01) { voicePool_.setAftertouch(v01); }

    // Ustawia parametr modulu we WSZYSTKICH 32 głosach (wywolywac z UI thread)
    void setModuleParam(int moduleId, int paramIdx, float value);

    // ── Arpeggiator — bezpieczne z UI thread ─────────────────────────────────
    void setArpEnabled (bool e)                 { arp_.setEnabled(e); }
    void setArpBPM     (float bpm)              { arp_.setBPM(bpm); }
    void setArpDivision(int d)                  { arp_.setDivision(d); }
    void setArpPattern (Arpeggiator::Pattern p) { arp_.setPattern(p); }
    void setArpOctaves (int n)                  { arp_.setOctaves(n); }
    bool  isArpEnabled ()               const   { return arp_.isEnabled(); }
    float getArpBPM    ()               const   { return arp_.getBPM(); }

    // ── Sequencer — bezpieczne z UI thread ───────────────────────────────────
    Sequencer& sequencer() { return seq_; }

private:
    VoicePool    voicePool_;
    Arpeggiator  arp_;
    Sequencer    seq_;
    juce::MidiMessageCollector midiCollector_;

    double sampleRate_ = 96000.0;
    int    blockSize_  = 64;

    // Bufor stereo wyjściowy (akumulacja głosów)
    std::vector<float> mixL_, mixR_;
};

} // namespace synth
