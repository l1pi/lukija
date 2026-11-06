#pragma once

#include "voicecatalog.h"

#include <QCryptographicHash>
#include <QFile>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QUrl>

class QNetworkReply;

namespace lukija {

// Downloads a catalogue voice into the voices directory, one at a time. The
// config comes first, then the model; both are checked against the
// catalogue's sizes and checksums. They're written as .part files and only
// renamed into place once both are good, so a voice is either fully
// installed or not at all.
class VoiceDownloader : public QObject
{
    Q_OBJECT

public:
    explicit VoiceDownloader(QObject *parent = nullptr);
    ~VoiceDownloader() override;

    // Where voices come from and go to, for tests.
    void setBaseUrl(const QUrl &url) { m_baseUrl = url; }
    void setDirectory(const QString &directory) { m_directory = directory; }
    QString directory() const { return m_directory; }

    bool isBusy() const { return m_voice.isValid(); }
    QString voiceId() const { return m_voice.id; }
    qint64 received() const { return m_done + m_received; }
    qint64 total() const { return m_voice.configSize + m_voice.modelSize; }

    // False if another download is running.
    bool start(const CatalogVoice &voice);
    void cancel();

signals:
    void progressChanged();
    void finished(const QString &voiceId);
    void failed(const QString &voiceId, const QString &error);

private:
    enum Step { Config, Model };

    QString targetPath(Step step) const;
    QString partPath(Step step) const;
    void fetch(Step step);
    void onReadyRead();
    void onFinished();
    void fail(const QString &error);
    void reset(bool removeParts);  // and tells
    void clear(bool removeParts);

    QNetworkAccessManager m_network;
    QUrl m_baseUrl;
    QString m_directory;

    CatalogVoice m_voice;
    Step m_step = Config;
    QPointer<QNetworkReply> m_reply;
    QFile m_file;
    QCryptographicHash m_hash{QCryptographicHash::Sha256};
    qint64 m_received = 0;  // of the current file
    qint64 m_done = 0;      // files already finished
};

} // namespace lukija
