#include "voicelibrary.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace lukija {

QString VoiceLibrary::voicesDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/voices");
}

QList<VoiceInfo> VoiceLibrary::installedVoices(const QString &directory)
{
    QList<VoiceInfo> voices;
    const QFileInfoList models = QDir(directory).entryInfoList({QStringLiteral("*.onnx")},
                                                               QDir::Files, QDir::Name);
    for (const QFileInfo &model : models) {
        const VoiceInfo voice = fromModelPath(model.absoluteFilePath());
        if (voice.isValid())
            voices.append(voice);
    }
    return voices;
}

VoiceInfo VoiceLibrary::find(const QString &idOrPath, const QString &directory)
{
    if (idOrPath.endsWith(QLatin1String(".onnx")))
        return fromModelPath(idOrPath);
    return fromModelPath(directory + QLatin1Char('/') + idOrPath + QStringLiteral(".onnx"));
}

VoiceInfo VoiceLibrary::fromModelPath(const QString &modelPath)
{
    VoiceInfo voice;
    const QString configPath = modelPath + QStringLiteral(".json");
    QFile config(configPath);
    if (!QFileInfo(modelPath).isFile() || !config.open(QIODevice::ReadOnly))
        return voice;

    const QJsonObject json = QJsonDocument::fromJson(config.readAll()).object();
    voice.id = QFileInfo(modelPath).completeBaseName();
    voice.language = json.value(QStringLiteral("language")).toObject()
                         .value(QStringLiteral("family")).toString();
    voice.modelPath = modelPath;
    voice.configPath = configPath;
    return voice;
}

QString VoiceLibrary::defaultVoiceId(const QString &language)
{
    if (language == QLatin1String("fi"))
        return QStringLiteral("fi_FI-harri-medium");
    return QStringLiteral("en_US-ljspeech-medium");
}

VoiceInfo VoiceLibrary::voiceFor(const QString &language, const QString &preferredId,
                                 const QString &directory)
{
    for (const QString &id : {preferredId, defaultVoiceId(language)}) {
        if (id.isEmpty())
            continue;
        const VoiceInfo voice = find(id, directory);
        if (voice.isValid() && voice.language == language)
            return voice;
    }

    const QList<VoiceInfo> installed = installedVoices(directory);
    for (const VoiceInfo &candidate : installed) {
        if (candidate.language == language)
            return candidate;
    }
    return VoiceInfo();
}

} // namespace lukija
