#!/usr/bin/env python3
"""
pch2_to_json.py — Bridge: Nord Modular G2 .pch2 → SynthApp JSON
Wymaga: pip install g2ools

Użycie:
  python3 pch2_to_json.py patch.pch2 [output.json]
  python3 pch2_to_json.py patch.pch2  # → stdout
"""

import sys
import json
import os

# ── Mapowanie typów modułów G2 → SynthApp ────────────────────────────────────
# Klucz: nazwa z g2ools (typ modułu G2)
# Wartość: nazwa w SynthApp (lub None = nieobsługiwany)
MODULE_MAP = {
    # Oscylatory
    "OscA":        "OscA",
    "OscB":        "OscB",
    "OscC":        "OscC",
    "OscD":        "OscD",
    "Noise":       "Noise",

    # Filtry
    "FltLP":       "FltLP",
    "FltClassic":  "FltLP",    # Classic Moog-style → FltLP
    "FltNord":     "FltNord",
    "FltHP":       "FltNord",  # HP mode via params
    "FltBP":       "FltNord",  # BP mode via params
    "FltNotch":    "FltNord",  # Notch mode via params
    "FltVoice":    "FltVoice",
    "FltPhase":    "FltPhase",

    # Koperty
    "EnvADSR":     "EnvADSR",
    "EnvAD":       "EnvAD",
    "EnvFollow":   "EnvFollow",

    # LFO
    "LfoA":        "LfoA",
    "LfoB":        "LfoB",
    "LfoC":        "LfoA",  # LfoC → LfoA (wavetable nie obsługiwane)

    # Sekwencery
    "SeqNote":     "SeqNote",
    "SeqCtrl":     "SeqCtrl",
    "ClkGen":      "ClkGen",
    "ClkDiv":      "ClkDiv",

    # Efekty
    "FxReverb":    "FxReverb",
    "FxDelay":     "FxDelay",
    "FxChorus":    "FxChorus",
    "FxFlanger":   "FxChorus",  # Flanger → Chorus (similar)
    "FxDistort":   "FxDistort",
    "FxComp":      "FxCompressor",
    "FxEQ":        "FxEQ",

    # Miksery
    "Mix2-1":      "Mix4to1",
    "Mix4-1":      "Mix4to1",
    "Mix8-1":      "Mix8to1",
    "Crossfade":   "Mix4to1",  # przybliżenie
    "Pan":         "Pan",
    "VCA":         "VCA",
    "Out":         "Out",

    # MIDI
    "KeyNote":     "KeyNote",
    "KeyVelo":     "KeyVelo",
    "KeyAfter":    "KeyAfter",
    "PitchBend":   "PitchBnd",
    "ModWheel":    "ModWheel",
}

# ── Mapowanie kolorów kabli G2 → SynthApp ────────────────────────────────────
CABLE_COLOR_MAP = {
    0: "audio",    # żółty
    1: "cv",       # niebieski
    2: "gate",     # pomarańczowy
}


def convert_patch(pch2_path: str) -> dict:
    """Konwertuje .pch2 do słownika SynthApp JSON."""
    try:
        import nm2
        patch = nm2.Pch2File(pch2_path)
    except ImportError:
        try:
            # Próba alternatywnej biblioteki
            import g2ools.nm2 as nm2
            patch = nm2.Pch2File(pch2_path)
        except ImportError:
            # Fallback: prosty parser binarny .pch2
            return convert_patch_fallback(pch2_path)

    result = {
        "version": "g2-import",
        "source_file": os.path.basename(pch2_path),
        "slots": []
    }

    for slot_idx, slot in enumerate(patch.slots):
        if slot is None:
            continue

        modules = []
        unmapped = []

        for m in slot.modules:
            synth_type = MODULE_MAP.get(m.type_name)
            if synth_type is None:
                unmapped.append(m.type_name)
                continue

            mod_entry = {
                "id":       m.index,
                "type":     synth_type,
                "original": m.type_name,
                "params":   {p.name: p.value for p in m.params},
                "position": {"row": getattr(m, "row", 0),
                             "col": getattr(m, "col", 0)},
            }
            modules.append(mod_entry)

        cables = []
        for c in slot.cables:
            cable_type = CABLE_COLOR_MAP.get(getattr(c, "color", 0), "audio")
            cables.append({
                "from": {"module": c.module_from, "port": c.output},
                "to":   {"module": c.module_to,   "port": c.input},
                "type": cable_type,
            })

        slot_entry = {
            "slot_index": slot_idx,
            "modules":    modules,
            "cables":     cables,
        }
        if unmapped:
            slot_entry["unmapped_modules"] = list(set(unmapped))

        result["slots"].append(slot_entry)

    return result


def convert_patch_fallback(pch2_path: str) -> dict:
    """
    Fallback gdy g2ools nie jest dostępny.
    Zwraca pusty szablon z informacją o błędzie.
    """
    return {
        "version":     "g2-import-failed",
        "source_file": os.path.basename(pch2_path),
        "error":       "g2ools not installed. Run: pip install g2ools",
        "install_hint": "pip install g2ools",
        "slots": []
    }


def main():
    if len(sys.argv) < 2:
        print("Użycie: pch2_to_json.py <plik.pch2> [wyjście.json]",
              file=sys.stderr)
        sys.exit(1)

    input_path  = sys.argv[1]
    output_path = sys.argv[2] if len(sys.argv) > 2 else None

    if not os.path.exists(input_path):
        print(f"Błąd: plik nie istnieje: {input_path}", file=sys.stderr)
        sys.exit(1)

    result = convert_patch(input_path)
    json_str = json.dumps(result, indent=2, ensure_ascii=False)

    if output_path:
        with open(output_path, "w", encoding="utf-8") as f:
            f.write(json_str)
        print(f"Zapisano: {output_path}", file=sys.stderr)
    else:
        print(json_str)


if __name__ == "__main__":
    main()
