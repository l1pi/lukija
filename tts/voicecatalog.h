#pragma once

#include <QByteArray>
#include <QList>
#include <QString>
#include <QUrl>

namespace lukija {

// A voice the app offers to download: a Piper voice from
// huggingface.co/rhasspy/piper-voices, pinned to one revision and checked
// against these sizes and checksums.
struct CatalogVoice {
    QString id;        // e.g. "fi_FI-harri-medium"; files are <id>.onnx and <id>.onnx.json
    QString language;  // language family, e.g. "fi"
    QString name;      // shown to the user, e.g. "Harri"
    QString licence;   // of the recordings it was trained on
    qint64 modelSize = 0;
    QByteArray modelSha256;  // hex
    qint64 configSize = 0;
    QByteArray configSha256;

    bool isValid() const { return !id.isEmpty(); }
};

class VoiceCatalog
{
public:
    // Where the voice files are downloaded from.
    static QUrl defaultBaseUrl();

    static QList<CatalogVoice> voices();
    static CatalogVoice find(const QString &id);

    // <base>/<family>/<code>/<name>/<quality>/<file>, the piper-voices layout.
    static QUrl fileUrl(const QUrl &base, const QString &id, const QString &file);
    static QUrl modelUrl(const CatalogVoice &voice, const QUrl &base = defaultBaseUrl());
    static QUrl configUrl(const CatalogVoice &voice, const QUrl &base = defaultBaseUrl());
    // An MP3 of the voice reading a sample text, to listen to before downloading.
    static QUrl sampleUrl(const CatalogVoice &voice, const QUrl &base = defaultBaseUrl());
};

} // namespace lukija
