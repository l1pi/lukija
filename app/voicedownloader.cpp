#include "voicedownloader.h"

#include "voicelibrary.h"

#include <QDir>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStorageInfo>

namespace lukija {

namespace {

// Room to spare beyond the voice itself.
constexpr qint64 SpareBytes = 50 * 1024 * 1024;

} // namespace

VoiceDownloader::VoiceDownloader(QObject *parent)
    : QObject(parent)
    , m_baseUrl(VoiceCatalog::defaultBaseUrl())
    , m_directory(VoiceLibrary::voicesDirectory())
{
}

VoiceDownloader::~VoiceDownloader()
{
    // No signals: whoever listens may be half destroyed already.
    clear(true);
}

bool VoiceDownloader::start(const CatalogVoice &voice)
{
    if (isBusy() || !voice.isValid())
        return false;

    m_voice = voice;
    m_done = 0;
    m_received = 0;
    emit progressChanged();

    if (!QDir().mkpath(m_directory)) {
        fail(tr("Can't create the folder %1").arg(m_directory));
        return true;
    }
    const QStorageInfo storage(m_directory);
    if (storage.isValid() && storage.bytesAvailable() < total() + SpareBytes) {
        fail(tr("Not enough free space: the voice needs %n MB", "", int(total() / 1000000)));
        return true;
    }
    fetch(Config);
    return true;
}

void VoiceDownloader::cancel()
{
    reset(true);
}

QString VoiceDownloader::targetPath(Step step) const
{
    return m_directory + QLatin1Char('/') + m_voice.id
           + (step == Config ? QStringLiteral(".onnx.json") : QStringLiteral(".onnx"));
}

QString VoiceDownloader::partPath(Step step) const
{
    return targetPath(step) + QStringLiteral(".part");
}

void VoiceDownloader::fetch(Step step)
{
    m_step = step;
    m_received = 0;
    m_hash.reset();

    m_file.setFileName(partPath(step));
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        fail(tr("Can't write %1").arg(m_file.fileName()));
        return;
    }

    QNetworkRequest request(step == Config ? VoiceCatalog::configUrl(m_voice, m_baseUrl)
                                           : VoiceCatalog::modelUrl(m_voice, m_baseUrl));
    // Hugging Face redirects the model to its CDN.
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
#else
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#endif
    m_reply = m_network.get(request);
    connect(m_reply.data(), &QNetworkReply::readyRead, this, &VoiceDownloader::onReadyRead);
    connect(m_reply.data(), &QNetworkReply::finished, this, &VoiceDownloader::onFinished);
}

void VoiceDownloader::onReadyRead()
{
    if (!m_reply)
        return;
    const QByteArray data = m_reply->readAll();
    const qint64 expected = m_step == Config ? m_voice.configSize : m_voice.modelSize;
    if (m_received + data.size() > expected) {
        fail(tr("The download is bigger than expected"));
        return;
    }
    if (m_file.write(data) != data.size()) {
        fail(tr("Can't write %1").arg(m_file.fileName()));
        return;
    }
    m_hash.addData(data);
    m_received += data.size();
    emit progressChanged();
}

void VoiceDownloader::onFinished()
{
    if (!m_reply)
        return;
    onReadyRead();
    if (!m_reply)
        return;  // failed while reading

    const QNetworkReply::NetworkError error = m_reply->error();
    const QString errorString = m_reply->errorString();
    m_reply->deleteLater();
    m_reply = nullptr;
    m_file.close();

    if (error != QNetworkReply::NoError) {
        fail(errorString);
        return;
    }
    const qint64 expectedSize = m_step == Config ? m_voice.configSize : m_voice.modelSize;
    const QByteArray expectedHash = m_step == Config ? m_voice.configSha256 : m_voice.modelSha256;
    if (m_received != expectedSize || m_hash.result().toHex() != expectedHash) {
        fail(tr("The downloaded file is damaged. Try again."));
        return;
    }

    if (m_step == Config) {
        m_done = m_received;
        m_received = 0;
        fetch(Model);
        return;
    }

    // The model last: once it's in place, the voice counts as installed.
    for (Step step : {Config, Model}) {
        QFile::remove(targetPath(step));
        if (!QFile::rename(partPath(step), targetPath(step))) {
            fail(tr("Can't write %1").arg(targetPath(step)));
            return;
        }
    }
    const QString id = m_voice.id;
    reset(false);
    emit finished(id);
}

void VoiceDownloader::fail(const QString &error)
{
    const QString id = m_voice.id;
    reset(true);
    emit failed(id, error);
}

void VoiceDownloader::reset(bool removeParts)
{
    const bool wasBusy = isBusy();
    clear(removeParts);
    if (wasBusy)
        emit progressChanged();
}

void VoiceDownloader::clear(bool removeParts)
{
    if (m_reply) {
        QNetworkReply *reply = m_reply;
        m_reply = nullptr;
        reply->disconnect(this);
        reply->abort();
        reply->deleteLater();
    }
    m_file.close();
    if (removeParts && m_voice.isValid()) {
        QFile::remove(partPath(Config));
        QFile::remove(partPath(Model));
    }
    m_voice = CatalogVoice();
    m_done = 0;
    m_received = 0;
}

} // namespace lukija
