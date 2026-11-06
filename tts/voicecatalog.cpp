#include "voicecatalog.h"

namespace lukija {

namespace {

// A revision of huggingface.co/rhasspy/piper-voices, so the files can't
// change under the checksums.
const char Revision[] = "e7229f7d33a10e145e109b886d661984201ac3c4";

CatalogVoice voice(const char *id, const char *language, const char *name, const char *licence,
                   qint64 modelSize, const char *modelSha256, qint64 configSize,
                   const char *configSha256)
{
    CatalogVoice result;
    result.id = QString::fromLatin1(id);
    result.language = QString::fromLatin1(language);
    result.name = QString::fromUtf8(name);
    result.licence = QString::fromUtf8(licence);
    result.modelSize = modelSize;
    result.modelSha256 = modelSha256;
    result.configSize = configSize;
    result.configSha256 = configSha256;
    return result;
}

} // namespace

QUrl VoiceCatalog::defaultBaseUrl()
{
    return QUrl(QStringLiteral("https://huggingface.co/rhasspy/piper-voices/resolve/")
                + QString::fromLatin1(Revision));
}

QList<CatalogVoice> VoiceCatalog::voices()
{
    // Medium quality: the high voices are too slow on a phone (docs/spikes.md).
    // Only voices trained on freely licensed recordings: en_US-lessac, for
    // one, comes from recordings licensed for research only.
    return {
        voice("fi_FI-harri-medium", "fi", "Harri", "CC0",
              63201294, "a44167faa34caed940e4fcad139fcc35922266b2593bcebe77701774c0fb2389",
              4873, "3f9c9f76f74adf1fbe7279e41eea17d6610757e45effd6808bbea6be74b8916d"),
        voice("en_US-ljspeech-medium", "en", "LJ Speech", "Public domain",
              63531379, "6f52a751e2349abe7a76735eb09dc1875298c77ea2342ffd2fef79ff81b87f22",
              4972, "141d612cc0a95ed7efc1ca936b845c2364967f2e9217c5dbfcf69fc4d6c65860"),
        voice("en_US-kristin-medium", "en", "Kristin", "Public domain",
              63531379, "5849957f929cbf720c258f8458692d6103fff2f0e3d3b19c8259474bb06a18d4",
              4968, "5681426d4aead22195de70531eeeeddb46493cfaffc5764b2ea3db73428b651c"),
        voice("en_US-norman-medium", "en", "Norman", "Public domain",
              63531379, "b9739443232a80a59c7d18810dd856899bf16a7964725f5ab81ea49b1351cb71",
              4968, "6c2db7f558a4a8deb9fe822583c1c5105f6c4e834dd0f9de8ad17a888ee9fe1d"),
        voice("en_GB-cori-medium", "en", "Cori", "Public domain",
              63531379, "1899f98e5fb8310154f3c2973f4b8a929ba7245e722b3d3a85680b833d95f10d",
              4966, "e262c16d7f192f69d4edd6b4ef8a5915379e67495fcc402f1ab15eeb33da3d36"),
    };
}

CatalogVoice VoiceCatalog::find(const QString &id)
{
    const QList<CatalogVoice> all = voices();
    for (const CatalogVoice &voice : all) {
        if (voice.id == id)
            return voice;
    }
    return CatalogVoice();
}

QUrl VoiceCatalog::fileUrl(const QUrl &base, const QString &id, const QString &file)
{
    // e.g. en_GB-cori-medium: en/en_GB/cori/medium
    const QString code = id.section(QLatin1Char('-'), 0, 0);
    const QString quality = id.section(QLatin1Char('-'), -1);
    const QString name = id.section(QLatin1Char('-'), 1, -2);
    const QString family = code.section(QLatin1Char('_'), 0, 0);

    QUrl url = base;
    url.setPath(base.path() + QStringLiteral("/%1/%2/%3/%4/%5")
                                  .arg(family, code, name, quality, file));
    return url;
}

QUrl VoiceCatalog::modelUrl(const CatalogVoice &voice, const QUrl &base)
{
    return fileUrl(base, voice.id, voice.id + QStringLiteral(".onnx"));
}

QUrl VoiceCatalog::configUrl(const CatalogVoice &voice, const QUrl &base)
{
    return fileUrl(base, voice.id, voice.id + QStringLiteral(".onnx.json"));
}

QUrl VoiceCatalog::sampleUrl(const CatalogVoice &voice, const QUrl &base)
{
    return fileUrl(base, voice.id, QStringLiteral("samples/speaker_0.mp3"));
}

} // namespace lukija
