#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace synth {

// ── Typy sygnałów (jak kable w G2) ───────────────────────────────────────────
enum class SignalType : uint8_t {
    Audio,  // żółty kabel  — pełna częstotliwość audio
    CV,     // niebieski    — wolno zmieniające się wartości kontrolne
    Gate,   // pomarańczowy — sygnały logiczne 0/1
};

// ── Opis pojedynczego portu ───────────────────────────────────────────────────
struct PortInfo {
    std::string name;
    SignalType  type;
    bool        isOutput;
};

// ── Bufor sygnału ─────────────────────────────────────────────────────────────
// Jeden bufor na port, rozmiar = blockSize (max 512 sampli)
struct SignalBuffer {
    static constexpr int kMaxBlock = 512;
    float data[kMaxBlock] = {};

    void clear(int n)       { std::fill(data, data + n, 0.0f); }
    float* ptr()            { return data; }
    const float* ptr() const{ return data; }
};

// ── Interfejs bazowy każdego modułu ──────────────────────────────────────────
class Module {
public:
    virtual ~Module() = default;

    // Konfiguracja przed pierwszym użyciem lub po zmianie sampleRate/blockSize
    virtual void prepare(double sampleRate, int blockSize) = 0;

    // Resetuje stan wewnętrzny (np. przy Note Off, nowy głos)
    virtual void reset() {}

    // Przetwarzanie audio — wywoływane w audio thread
    // ZAKAZ: alokacji pamięci, mutexów, syscalli
    //
    // inputs  — tablica wskaźników do buforów wejściowych (może być nullptr)
    // outputs — tablica wskaźników do buforów wyjściowych
    // Kolejność portów = kolejność z getPorts() filtrowana po isOutput
    virtual void process(const float* const* inputs,
                         float* const*       outputs,
                         int                 numSamples) = 0;

    // Parametry jako wartości znormalizowane [0..1]
    virtual void  setParam(int index, float value) = 0;
    virtual float getParam(int index) const = 0;
    virtual int   getNumParams() const = 0;

    // Metadane — wywoływane tylko z message thread
    virtual std::vector<PortInfo> getPorts() const = 0;
    virtual std::string           getName()  const = 0;

    // Unikalne ID instancji (nadawane przez ModuleGraph)
    int id = -1;
};

} // namespace synth
