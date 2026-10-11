#pragma once

#include "ttsengine.h"

struct piper_synthesizer;

namespace lukija {

// Piper neural TTS through libpiper's C API.
class PiperEngine : public TtsEngine
{
public:
    explicit PiperEngine(const QString &espeakDataPath = defaultEspeakDataPath());
    ~PiperEngine() override;

    PiperEngine(const PiperEngine &) = delete;
    PiperEngine &operator=(const PiperEngine &) = delete;

    bool load(const VoiceInfo &voice, QString *error) override;
    int sampleRate() const override { return m_sampleRate; }
    bool synthesize(const QString &text, const SynthOptions &options, QVector<float> *pcm,
                    const std::atomic<bool> &cancel) override;

    // $LUKIJA_ESPEAK_DATA if set, then the espeak-ng-data installed with the
    // app, then the one in the build tree.
    static QString defaultEspeakDataPath();

    // onnxruntime threads for voices loaded from now on ($LUKIJA_PIPER_THREADS,
    // read by our patched libpiper). The default is 2: on the Jolla Phone that
    // nearly halves synthesis time for 12% more CPU time (docs/spikes.md).
    static void setThreadCount(int threads);
    static constexpr int DefaultThreadCount = 2;

private:
    piper_synthesizer *m_synth = nullptr;
    QString m_espeakDataPath;
    int m_sampleRate = 22050;
};

} // namespace lukija
