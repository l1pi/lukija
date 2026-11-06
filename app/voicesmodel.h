#pragma once

#include "voicedownloader.h"

#include <QAbstractListModel>

namespace lukija {

// The voices page: the catalogue's voices and any others installed by hand,
// by language. Downloads, removes, and chooses the voice for a language.
class VoicesModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(int installedCount READ installedCount NOTIFY installedCountChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)

public:
    enum Role {
        VoiceIdRole = Qt::UserRole + 1,
        NameRole,
        LanguageRole,      // "fi"
        LanguageNameRole,  // "Finnish"
        CountryRole,       // "Finland"
        SizeRole,          // bytes to download
        LicenceRole,
        SampleUrlRole,
        InCatalogRole,
        InstalledRole,
        InUseRole,  // the voice that reads this language
        DownloadingRole,
        ProgressRole,  // 0–1
    };

    explicit VoicesModel(QObject *parent = nullptr);

    // Where voices are installed and downloaded from, for tests.
    void setDirectory(const QString &directory);
    void setBaseUrl(const QUrl &url) { m_downloader.setBaseUrl(url); }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool isBusy() const { return m_downloader.isBusy(); }
    int installedCount() const;
    QString errorString() const { return m_errorString; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void download(const QString &voiceId);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void remove(const QString &voiceId);
    // Reads its language with this voice from now on.
    Q_INVOKABLE void choose(const QString &voiceId);

signals:
    void busyChanged();
    void installedCountChanged();
    void errorStringChanged();
    void voicesChanged();  // installed, removed or chosen

private:
    struct Row {
        CatalogVoice voice;  // for voices outside the catalogue, only id, name and language
        bool inCatalog = false;
        bool installed = false;
        bool inUse = false;
    };

    int rowOf(const QString &voiceId) const;
    void onProgress();
    void onFinished();
    void onFailed(const QString &voiceId, const QString &error);
    void setErrorString(const QString &error);

    VoiceDownloader m_downloader;
    QList<Row> m_rows;
    QString m_errorString;
    QString m_downloading;  // id, to notice when it changes
    int m_percent = -1;     // last progress reported
};

} // namespace lukija
