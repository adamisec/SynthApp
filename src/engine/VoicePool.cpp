#include "VoicePool.h"
#include <algorithm>
#include <cassert>
#include <limits>

namespace synth {

void VoicePool::prepare(double sampleRate, int blockSize, GraphFactory factory) {
    sampleRate_ = sampleRate;
    blockSize_  = blockSize;
    factory_    = factory;

    for (int i = 0; i < kMaxVoices; ++i) {
        voices_[i].voiceIdx = i;
        voices_[i].note     = Voice::kInactive;
        factory_(voices_[i].graph);
        voices_[i].graph.recompile(sampleRate, blockSize);
    }
}

// ── Voice allocation ──────────────────────────────────────────────────────────

Voice* VoicePool::allocateVoice(int note) {
    // 1. Retrigger: ten sam głos gra tę samą nutę
    if (auto* v = findVoice(note)) return v;

    // 2. Wolny głos (ani aktywny ani w release)
    for (auto& v : voices_)
        if (v.isFree()) return &v;

    // 3. Preferuj głosy w release — już zanikają, najlepsi kandydaci
    for (auto& v : voices_)
        if (v.releasing) { v.releasing = false; return &v; }

    // 4. Voice stealing — najstarszy aktywny głos
    int      oldestIdx = 0;
    uint32_t oldestAge = std::numeric_limits<uint32_t>::max();
    for (int i = 0; i < kMaxVoices; ++i) {
        if (voiceAge_[i] < oldestAge) {
            oldestAge = voiceAge_[i];
            oldestIdx = i;
        }
    }
    return &voices_[oldestIdx];
}

Voice* VoicePool::findVoice(int note) {
    for (auto& v : voices_)
        if (v.note == note) return &v;
    return nullptr;
}

// ── MIDI events ───────────────────────────────────────────────────────────────

void VoicePool::noteOn(int note, float velocity, int /*sampleOffset*/) {
    Voice* v = allocateVoice(note);
    v->note     = note;
    v->velocity = velocity;
    voiceAge_[v->voiceIdx] = ageClock_++;

    // Wyślij NoteOn do grafu głosu przez dedykowany moduł KeyNote
    // (KeyNote czyta stały parametr "note" i "gate" ustawiany tu)
    v->releasing = false;  // anuluj ewentualne zanikanie

    if (auto* keyNote = v->graph.getModule(0 /* KeyNote zawsze id=0 */)) {
        keyNote->setParam(0, static_cast<float>(note) / 127.0f);   // pitch
        keyNote->setParam(1, velocity);                              // velocity
        keyNote->setParam(2, 1.0f);                                  // gate ON
    }
    // Nie recompilujemy — topologia grafu nie zmienia się przy zmianie nuty
}

void VoicePool::noteOff(int note, int /*sampleOffset*/) {
    Voice* v = findVoice(note);
    if (!v) return;

    if (sustainHeld_) {
        // Trzymamy gate — głos zostanie zwolniony dopiero po puszczeniu pedału
        v->sustained = true;
        return;
    }

    if (auto* keyNote = v->graph.getModule(0)) {
        keyNote->setParam(2, 0.0f);  // gate OFF
    }

    v->note      = Voice::kInactive;
    v->releasing = true;
}

void VoicePool::setSustain(bool held) {
    sustainHeld_ = held;
    if (!held) {
        // Puść wszystkie głosy trzymane przez pedał
        for (auto& v : voices_) {
            if (v.sustained) {
                v.sustained = false;
                if (auto* keyNote = v.graph.getModule(0))
                    keyNote->setParam(2, 0.0f);
                v.note      = Voice::kInactive;
                v.releasing = true;
            }
        }
    }
}

// ── Przetwarzanie ─────────────────────────────────────────────────────────────

void VoicePool::process(float* outputL, float* outputR, int numSamples) {
    for (auto& voice : voices_) {
        if (!voice.isActive()) continue;
        voice.graph.process(numSamples);

        const float* outL = voice.graph.getOutputBuffer(outModuleId_, 0);
        const float* outR = voice.graph.getOutputBuffer(outModuleId_, 1);
        if (!outL) { voice.releasing = false; continue; }
        if (!outR) outR = outL;  // mono → stereo

        float level = 0.0f;
        for (int i = 0; i < numSamples; ++i) {
            outputL[i] += outL[i];
            outputR[i] += outR[i];
            level += std::abs(outL[i]);
        }

        // Głos w Release: wyłącz gdy amplituda spada poniżej progu ciszy
        if (voice.releasing && level < 1e-5f * numSamples)
            voice.releasing = false;
    }
}

void VoicePool::setGlobalModuleParam(int moduleId, int paramIdx, float value) {
    for (auto& v : voices_) {
        if (auto* mod = v.graph.getModule(moduleId))
            mod->setParam(paramIdx, value);
    }
}

int VoicePool::activeVoiceCount() const {
    int count = 0;
    for (auto& v : voices_)
        if (v.isActive()) count++;
    return count;
}

} // namespace synth
