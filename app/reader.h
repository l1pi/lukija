#pragma once

#include "library.h"
#include "playbackcontroller.h"
#include "sentencemodel.h"

#include <QFutureWatcher>
#include <QObject>
#include <QTimer>

#include <memory>

namespace lukija {

// The open document: loads it in the background (from the extraction cache
// when possible), resumes where the user stopped, and keeps the position saved.
class Reader : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY statusChanged)
    Q_PROPERTY(QString path READ path NOTIFY documentChanged)
    Q_PROPERTY(QString title READ title NOTIFY documentChanged)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY documentChanged)
    Q_PROPERTY(QObject *sentences READ sentences CONSTANT)
    Q_PROPERTY(QObject *player READ player CONSTANT)

public:
    enum Status { Empty, Loading, Ready, Error };
    Q_ENUM(Status)

    explicit Reader(Library *library, QObject *parent = nullptr);
    ~Reader() override;

    Status status() const { return m_status; }
    QString errorString() const { return m_errorString; }
    QString path() const { return m_path; }
    QString title() const;
    int pageCount() const;
    SentenceModel *sentences() { return &m_sentences; }
    PlaybackController *player() { return &m_player; }

    // A local path or a file:// URL.
    Q_INVOKABLE void open(const QString &pathOrUrl);
    Q_INVOKABLE void close();
    Q_INVOKABLE QString sentenceText(int index) const;

public slots:
    void savePosition();

signals:
    void statusChanged();
    void documentChanged();
    void opened();  // a document was added to the library or reopened

private:
    struct LoadResult {
        QString path;
        QString hash;
        std::shared_ptr<ReadingDocument> document;
        QString error;
    };
    static LoadResult load(const QString &path);
    void startLoading(const QString &path);
    void onLoaded();
    void setStatus(Status status, const QString &error = QString());

    Library *m_library;
    PlaybackController m_player;
    SentenceModel m_sentences;
    QFutureWatcher<LoadResult> m_watcher;
    QTimer m_saveTimer;

    Status m_status = Empty;
    QString m_errorString;
    QString m_path;             // the open (or loading) document
    QString m_pendingPath;      // requested while another load was running
    std::shared_ptr<const ReadingDocument> m_document;
    int m_documentId = -1;
};

} // namespace lukija
