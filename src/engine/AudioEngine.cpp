#include "AudioEngine.h"
#include "modules/oscillators/OscA.h"
#include "modules/oscillators/OscB.h"
#include "modules/filters/FltNord.h"
#include "modules/envelopes/EnvADSR.h"
#include "modules/mixers/VCA.h"
#include "modules/mixers/Mix4to1.h"
#include "modules/mixers/Out.h"
#include "modules/midi/KeyNote.h"
#include "modules/lfos/LfoA.h"
#include "modules/effects/FxChorus.h"
#include "modules/effects/FxReverb.h"
#include "PluginEditor.h"

namespace synth {

AudioEngine::AudioEngine()
    : AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

void AudioEngine::prepareToPlay(double sampleRate, int blockSize) {
    sampleRate_ = sampleRate;
    blockSize_  = blockSize;

    midiCollector_.reset(sampleRate);

    mixL_.resize(blockSize);
    mixR_.resize(blockSize);

    // ── "Space Pad" patch ────────────────────────────────────────────────────
    // OscA (pila) + OscB (pila, +10 centow detune)
    //   → Mix4to1
    //   → FltNord LP24 modulowany przez LfoA (sinus na cutoff)
    //   → VCA sterowany EnvADSR (wolny atak, dluga release)
    //   → FxChorus → FxReverb
    //   → Out

    int outId = -1;
    auto factory = [&outId](ModuleGraph& g) {
        int keyId    = g.addModule(std::make_unique<KeyNote>());   // 0
        int oscAId   = g.addModule(std::make_unique<OscA>());      // 1
        int oscBId   = g.addModule(std::make_unique<OscB>());      // 2
        int mixId    = g.addModule(std::make_unique<Mix4to1>());   // 3
        int fltId    = g.addModule(std::make_unique<FltNord>());   // 4
        int lfoId    = g.addModule(std::make_unique<LfoA>());      // 5
        int envId    = g.addModule(std::make_unique<EnvADSR>());   // 6
        int vcaId    = g.addModule(std::make_unique<VCA>());       // 7
        int chorusId = g.addModule(std::make_unique<FxChorus>());  // 8
        int reverbId = g.addModule(std::make_unique<FxReverb>());  // 9
        outId        = g.addModule(std::make_unique<Out>());       // 10

        // ── Parametry ────────────────────────────────────────────────────────
        // OscA: piła
        g.getModule(oscAId)->setParam(OscA::kWaveform, 0.5f);   // Sawtooth

        // OscB: piła, +10 centow wyżej (thickening detune)
        g.getModule(oscBId)->setParam(OscB::kWaveform, 0.5f);   // Sawtooth
        g.getModule(oscBId)->setParam(OscB::kFine,     0.555f); // +10 centow

        // Mix4to1: OscA i OscB rowno
        g.getModule(mixId)->setParam(Mix4to1::kLevelA, 0.7f);
        g.getModule(mixId)->setParam(Mix4to1::kLevelB, 0.7f);

        // FltNord SVF LP 24dB — cutoff troche ponizej polowy, sredni rezonans
        g.getModule(fltId)->setParam(FltNord::kCutoff,    0.45f);
        g.getModule(fltId)->setParam(FltNord::kResonance, 0.35f);
        g.getModule(fltId)->setParam(FltNord::kSlope,     1.0f); // 24 dB/oct

        // LfoA: sinus, wolny (ok. 0.5 Hz), umiarkowana glebokosc
        g.getModule(lfoId)->setParam(LfoA::kRate,     0.2f);
        g.getModule(lfoId)->setParam(LfoA::kWaveform, 0.0f);  // Sine
        g.getModule(lfoId)->setParam(LfoA::kDepth,    0.22f);

        // EnvADSR: wolny atak (pad), dluga release
        g.getModule(envId)->setParam(EnvADSR::kAttack,  0.25f);
        g.getModule(envId)->setParam(EnvADSR::kDecay,   0.3f);
        g.getModule(envId)->setParam(EnvADSR::kSustain, 0.75f);
        g.getModule(envId)->setParam(EnvADSR::kRelease, 0.55f);

        // FxChorus: umiarkowany, "vintage" szerokosc
        g.getModule(chorusId)->setParam(FxChorus::kRate,  0.35f);
        g.getModule(chorusId)->setParam(FxChorus::kDepth, 0.6f);
        g.getModule(chorusId)->setParam(FxChorus::kDelay, 0.3f);
        g.getModule(chorusId)->setParam(FxChorus::kMix,   0.5f);

        // FxReverb: duzy pokoj, szeroki stereo, 45% wet
        g.getModule(reverbId)->setParam(FxReverb::kRoomSize, 0.8f);
        g.getModule(reverbId)->setParam(FxReverb::kDamp,     0.4f);
        g.getModule(reverbId)->setParam(FxReverb::kWidth,    0.9f);
        g.getModule(reverbId)->setParam(FxReverb::kMix,      0.45f);

        // ── Kable (output index → input index, jak w ModuleGraph::Cable) ────
        g.addCable({ keyId,    0, oscAId,   0 });  // PitchCV → OscA
        g.addCable({ keyId,    0, oscBId,   0 });  // PitchCV → OscB
        g.addCable({ oscAId,   0, mixId,    0 });  // OscA → Mix InA
        g.addCable({ oscBId,   0, mixId,    1 });  // OscB → Mix InB
        g.addCable({ mixId,    0, fltId,    0 });  // Mix → FltNord Audio
        g.addCable({ lfoId,    0, fltId,    1 });  // LFO → FltNord CutoffCV
        g.addCable({ keyId,    1, envId,    0 });  // Gate → EnvADSR
        g.addCable({ fltId,    0, vcaId,    0 });  // FltNord LP → VCA
        g.addCable({ envId,    0, vcaId,    1 });  // Env → VCA gain
        g.addCable({ vcaId,    0, chorusId, 0 });  // VCA → Chorus
        g.addCable({ chorusId, 0, reverbId, 0 });  // Chorus OutL → Reverb
        g.addCable({ reverbId, 0, outId,    0 });  // Reverb OutL → Out InL
        g.addCable({ reverbId, 1, outId,    1 });  // Reverb OutR → Out InR
    };

    voicePool_.prepare(sampleRate, blockSize, factory);
    voicePool_.setOutModuleId(outId);
    arp_.prepare(sampleRate);
    seq_.prepare(sampleRate);
}

void AudioEngine::setModuleParam(int moduleId, int paramIdx, float value) {
    for (int i = 0; i < VoicePool::kMaxVoices; ++i) {
        if (auto* mod = voicePool_.voiceGraph(i).getModule(moduleId))
            mod->setParam(paramIdx, value);
    }
}

void AudioEngine::noteOn(int midiNote, float velocity) {
    midiCollector_.addMessageToQueue(
        juce::MidiMessage::noteOn(1, midiNote,
                                  static_cast<uint8_t>(velocity * 127.f)));
}

void AudioEngine::noteOff(int midiNote) {
    midiCollector_.addMessageToQueue(
        juce::MidiMessage::noteOff(1, midiNote));
}

void AudioEngine::processBlock(juce::AudioBuffer<float>& buffer,
                                juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;

    int numSamples = buffer.getNumSamples();

    // Zdarzenia z wirtualnej klawiatury
    juce::MidiBuffer keyboardMidi;
    midiCollector_.removeNextBlockOfMessages(keyboardMidi, numSamples);
    for (const auto meta : keyboardMidi)
        midiMessages.addEvent(meta.getMessage(), meta.samplePosition);

    buffer.clear();

    std::fill(mixL_.begin(), mixL_.begin() + numSamples, 0.0f);
    std::fill(mixR_.begin(), mixR_.begin() + numSamples, 0.0f);

    for (const auto meta : midiMessages) {
        auto msg    = meta.getMessage();
        int  offset = meta.samplePosition;

        if (msg.isNoteOn()) {
            if (arp_.isEnabled())
                arp_.noteOn(msg.getNoteNumber(), msg.getVelocity() / 127.0f);
            else
                voicePool_.noteOn(msg.getNoteNumber(),
                                  msg.getVelocity() / 127.0f,
                                  offset);
        } else if (msg.isNoteOff()) {
            if (arp_.isEnabled())
                arp_.noteOff(msg.getNoteNumber());
            else
                voicePool_.noteOff(msg.getNoteNumber(), offset);
        } else if (msg.isPitchWheel()) {
            voicePool_.setPitchBend((msg.getPitchWheelValue() + 8192) / 16383.0f);
        } else if (msg.isChannelPressure()) {
            voicePool_.setAftertouch(msg.getChannelPressureValue() / 127.0f);
        } else if (msg.isController()) {
            int cc  = msg.getControllerNumber();
            float v = msg.getControllerValue() / 127.0f;
            if (cc == 1)   voicePool_.setModWheel(v);
            if (cc == 64)  voicePool_.setSustain(v > 0.5f);
        }
    }

    // Arpeggiator generuje timed events → VoicePool
    for (auto& evt : arp_.process(numSamples)) {
        if (evt.noteOn)
            voicePool_.noteOn (evt.note, evt.velocity,  evt.sampleOffset);
        else
            voicePool_.noteOff(evt.note, evt.sampleOffset);
    }

    // Sequencer generuje timed events → VoicePool
    for (auto& evt : seq_.process(numSamples)) {
        if (evt.noteOn)
            voicePool_.noteOn (evt.note, evt.velocity,  evt.sampleOffset);
        else
            voicePool_.noteOff(evt.note, evt.sampleOffset);
    }

    voicePool_.process(mixL_.data(), mixR_.data(), numSamples);

    if (buffer.getNumChannels() >= 1)
        buffer.addFrom(0, 0, mixL_.data(), numSamples);
    if (buffer.getNumChannels() >= 2)
        buffer.addFrom(1, 0, mixR_.data(), numSamples);
}

juce::AudioProcessorEditor* AudioEngine::createEditor() {
    return new PluginEditor(*this);
}

} // namespace synth
