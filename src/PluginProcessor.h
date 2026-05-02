#pragma once
#include "engine/AudioEngine.h"

// PluginProcessor jest cienką warstwą — cała logika w AudioEngine.
// JUCE wymaga tej nazwy dla automatycznego wykrycia pluginu.
using PluginProcessor = synth::AudioEngine;
