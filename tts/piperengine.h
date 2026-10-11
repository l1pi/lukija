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

    // $LUKIJA_ESPEAK_DATA if set, otherwise the espeak-ng-data that the build
    // installed next to libpiper.
    static QString defaultEspeakDataPath();

private:
    piper_synthesizer *m_synth = nullptr;
    QString m_espeakDataPath;
    int m_sampleRate = 22050;
};

} // namespace lukija
