#include "piperengine.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include <piper.h>

namespace lukija {

PiperEngine::PiperEngine(const QString &espeakDataPath)
    : m_espeakDataPath(espeakDataPath)
{
}

PiperEngine::~PiperEngine()
{
    if (m_synth)
        piper_free(m_synth);
}

QString PiperEngine::defaultEspeakDataPath()
{
    const QByteArray fromEnvironment = qgetenv("LUKIJA_ESPEAK_DATA");
    if (!fromEnvironment.isEmpty())
        return QFile::decodeName(fromEnvironment);
#ifdef LUKIJA_PIPER_ESPEAK_DATA
    return QStringLiteral(LUKIJA_PIPER_ESPEAK_DATA);
#else
    return QString();
#endif
}

bool PiperEngine::load(const VoiceInfo &voice, QString *error)
{
    if (m_synth) {
        piper_free(m_synth);
        m_synth = nullptr;
    }

    if (!QDir(m_espeakDataPath).exists()) {
        if (error)
            *error = QStringLiteral("espeak-ng data not found at '%1'").arg(m_espeakDataPath);
        return false;
    }

    QFile config(voice.configPath);
    if (!config.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QStringLiteral("Cannot read voice config '%1'").arg(voice.configPath);
        return false;
    }
    const QJsonObject audio = QJsonDocument::fromJson(config.readAll()).object()
                                  .value(QStringLiteral("audio")).toObject();
    m_sampleRate = audio.value(QStringLiteral("sample_rate")).toInt(22050);

    m_synth = piper_create(QFile::encodeName(voice.modelPath).constData(),
                           QFile::encodeName(voice.configPath).constData(),
                           QFile::encodeName(m_espeakDataPath).constData());
    if (!m_synth) {
        if (error)
            *error = QStringLiteral("Piper could not load voice '%1'").arg(voice.modelPath);
        return false;
    }
    return true;
}

bool PiperEngine::synthesize(const QString &text, const SynthOptions &options, QVector<float> *pcm,
                             const std::atomic<bool> &cancel)
{
    if (!m_synth)
        return false;

    piper_synthesize_options piperOptions = piper_default_synthesize_options(m_synth);
    piperOptions.length_scale /= qMax(0.1f, options.speed);

    if (piper_synthesize_start(m_synth, text.toUtf8().constData(), &piperOptions) != PIPER_OK)
        return false;

    piper_audio_chunk chunk;
    for (;;) {
        const int result = piper_synthesize_next(m_synth, &chunk);
        if (result != PIPER_OK && result != PIPER_DONE)
            return false;
        // The last chunk comes with PIPER_DONE and still carries audio.
        pcm->reserve(pcm->size() + int(chunk.num_samples));
        for (size_t i = 0; i < chunk.num_samples; ++i)
            pcm->append(chunk.samples[i]);
        if (result == PIPER_DONE)
            return true;
        if (cancel)
            return false;
    }
}

} // namespace lukija
