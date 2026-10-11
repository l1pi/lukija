#pragma once

#include <QString>
#include <QVector>

#include <atomic>

namespace lukija {

struct VoiceInfo {
    QString id;          // e.g. "fi_FI-harri-medium"
    QString language;    // language family, e.g. "fi"
    QString modelPath;   // .onnx
    QString configPath;  // .onnx.json

    bool isValid() const { return !modelPath.isEmpty(); }
};

struct SynthOptions {
    float speed = 1.0f;  // 1.0 = the voice's natural pace
};

class TtsEngine
{
public:
    virtual ~TtsEngine() = default;

    virtual bool load(const VoiceInfo &voice, QString *error) = 0;
    virtual int sampleRate() const = 0;

    // Called on the synthesis thread. Fills mono float PCM; returns false if
    // cancelled or failed.
    virtual bool synthesize(const QString &text, const SynthOptions &options,
                            QVector<float> *pcm, const std::atomic<bool> &cancel) = 0;
};

} // namespace lukija
