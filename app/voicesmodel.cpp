#include "voicesmodel.h"

#include "appsettings.h"
#include "playbackcontroller.h"
#include "voicelibrary.h"

#include <QFile>
#include <QLocale>
#include <QRegularExpression>
#include <QSet>

namespace lukija {

namespace {

// Qt 5.6 names are CamelCase, "UnitedStates"; newer ones have spaces.
QString words(QString name)
{
    static const QRegularExpression boundary(QStringLiteral("(\\p{Ll})(\\p{Lu})"));
    return name.replace(boundary, QStringLiteral("\\1 \\2"));
}

QString languageName(const QString &language)
{
    return words(QLocale::languageToString(QLocale(language).language()));
}

// From the id's locale part, e.g. "en_GB" in en_GB-cori-medium.
QString countryName(const QString &voiceId)
{
    const QLocale locale(voiceId.section(QLatin1Char('-'), 0, 0));
    return locale.country() == QLocale::AnyCountry ? QString()
                                                   : words(QLocale::countryToString(locale.country()));
}

} // namespace

VoicesModel::VoicesModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&m_downloader, &VoiceDownloader::progressChanged, this, &VoicesModel::onProgress);
    connect(&m_downloader, &VoiceDownloader::finished, this, &VoicesModel::onFinished);
    connect(&m_downloader, &VoiceDownloader::failed, this, &VoicesModel::onFailed);
    refresh();
}

void VoicesModel::setDirectory(const QString &directory)
{
    m_downloader.setDirectory(directory);
    refresh();
}

int VoicesModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant VoicesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return QVariant();
    const Row &row = m_rows.at(index.row());
    const bool downloading = m_downloader.voiceId() == row.voice.id;
    switch (role) {
    case VoiceIdRole:
        return row.voice.id;
    case NameRole:
        return row.voice.name;
    case LanguageRole:
        return row.voice.language;
    case LanguageNameRole:
        return languageName(row.voice.language);
    case CountryRole:
        return countryName(row.voice.id);
    case SizeRole:
        return row.voice.modelSize + row.voice.configSize;
    case LicenceRole:
        return row.voice.licence;
    case SampleUrlRole:
        return row.inCatalog ? VoiceCatalog::sampleUrl(row.voice) : QUrl();
    case InCatalogRole:
        return row.inCatalog;
    case InstalledRole:
        return row.installed;
    case InUseRole:
        return row.inUse;
    case DownloadingRole:
        return downloading;
    case ProgressRole:
        return downloading && m_downloader.total() > 0
                   ? qreal(m_downloader.received()) / m_downloader.total()
                   : 0.0;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> VoicesModel::roleNames() const
{
    return {
        {VoiceIdRole, "voiceId"},
        {NameRole, "name"},
        {LanguageRole, "language"},
        {LanguageNameRole, "languageName"},
        {CountryRole, "country"},
        {SizeRole, "size"},
        {LicenceRole, "licence"},
        {SampleUrlRole, "sampleUrl"},
        {InCatalogRole, "inCatalog"},
        {InstalledRole, "installed"},
        {InUseRole, "inUse"},
        {DownloadingRole, "downloading"},
        {ProgressRole, "progress"},
    };
}

int VoicesModel::installedCount() const
{
    int count = 0;
    for (const Row &row : m_rows)
        count += row.installed ? 1 : 0;
    return count;
}

void VoicesModel::refresh()
{
    const QString directory = m_downloader.directory();
    const int oldInstalled = installedCount();

    QList<Row> rows;
    QSet<QString> installedIds;
    const QList<VoiceInfo> installed = VoiceLibrary::installedVoices(directory);
    for (const VoiceInfo &voice : installed)
        installedIds.insert(voice.id);

    // The catalogue's order, then voices installed by hand, after the
    // catalogue's voices for the same language.
    const QList<CatalogVoice> catalog = VoiceCatalog::voices();
    for (const CatalogVoice &voice : catalog) {
        Row row;
        row.voice = voice;
        row.inCatalog = true;
        row.installed = installedIds.contains(voice.id);
        rows.append(row);
    }
    for (const VoiceInfo &voice : installed) {
        if (VoiceCatalog::find(voice.id).isValid())
            continue;
        Row row;
        row.voice.id = voice.id;
        // "Lessac (high)": the quality too, since there can be several.
        row.voice.name = voice.id.section(QLatin1Char('-'), 1, -2);
        if (!row.voice.name.isEmpty())
            row.voice.name[0] = row.voice.name.at(0).toUpper();
        const QString quality = voice.id.section(QLatin1Char('-'), -1);
        if (!quality.isEmpty() && quality != voice.id)
            row.voice.name += QStringLiteral(" (%1)").arg(quality);
        row.voice.language = voice.language;
        row.installed = true;
        int at = rows.size();
        while (at > 0 && rows.at(at - 1).voice.language != voice.language)
            --at;
        rows.insert(at > 0 ? at : rows.size(), row);
    }

    // The voice each language is read with.
    const AppSettings settings;
    for (Row &row : rows) {
        const QString preferred = settings.value(PlaybackController::voiceSettingKey(row.voice.language))
                                      .toString();
        row.inUse = row.installed
                    && VoiceLibrary::voiceFor(row.voice.language, preferred, directory).id == row.voice.id;
    }

    beginResetModel();
    m_rows = rows;
    endResetModel();
    if (installedCount() != oldInstalled)
        emit installedCountChanged();
}

void VoicesModel::download(const QString &voiceId)
{
    const CatalogVoice voice = VoiceCatalog::find(voiceId);
    if (!voice.isValid() || m_downloader.isBusy())
        return;
    setErrorString(QString());
    m_downloader.start(voice);
}

void VoicesModel::cancel()
{
    m_downloader.cancel();
}

void VoicesModel::remove(const QString &voiceId)
{
    if (voiceId.isEmpty() || voiceId == m_downloader.voiceId())
        return;
    const QString base = m_downloader.directory() + QLatin1Char('/') + voiceId;
    QFile::remove(base + QStringLiteral(".onnx"));
    QFile::remove(base + QStringLiteral(".onnx.json"));
    refresh();
    emit voicesChanged();
}

void VoicesModel::choose(const QString &voiceId)
{
    const int row = rowOf(voiceId);
    if (row < 0 || !m_rows.at(row).installed)
        return;
    AppSettings().setValue(PlaybackController::voiceSettingKey(m_rows.at(row).voice.language), voiceId);
    refresh();
    emit voicesChanged();
}

int VoicesModel::rowOf(const QString &voiceId) const
{
    for (int row = 0; row < m_rows.size(); ++row) {
        if (m_rows.at(row).voice.id == voiceId)
            return row;
    }
    return -1;
}

void VoicesModel::onProgress()
{
    // The download that started or stopped, and the one in progress: only
    // whole percents, rather than on every packet.
    const QString current = m_downloader.voiceId();
    const int percent = m_downloader.total() > 0
                            ? int(m_downloader.received() * 100 / m_downloader.total())
                            : 0;
    if (current == m_downloading && percent == m_percent)
        return;

    QStringList changed{m_downloading, current};
    const bool busyChanging = m_downloading.isEmpty() != current.isEmpty();
    m_downloading = current;
    m_percent = percent;
    for (const QString &id : changed) {
        const int row = rowOf(id);
        if (row >= 0)
            emit dataChanged(index(row), index(row), {DownloadingRole, ProgressRole});
    }
    if (busyChanging)
        emit busyChanged();
}

void VoicesModel::onFinished()
{
    refresh();
    emit voicesChanged();
}

void VoicesModel::onFailed(const QString &voiceId, const QString &error)
{
    const int row = rowOf(voiceId);
    const QString name = row >= 0 ? m_rows.at(row).voice.name : voiceId;
    //: Voices page: a download failed. %1 is the voice's name, %2 the reason.
    setErrorString(tr("Downloading %1 failed: %2").arg(name, error));
}

void VoicesModel::setErrorString(const QString &error)
{
    if (m_errorString == error)
        return;
    m_errorString = error;
    emit errorStringChanged();
}

} // namespace lukija
