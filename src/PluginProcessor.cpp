#include "PluginProcessor.h"

// Wymagana przez JUCE makro do rejestracji pluginu
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new synth::AudioEngine();
}
