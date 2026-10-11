#include "reader.h"

#include "pipeline.h"

#include <QFileInfo>
#include <QUrl>
#include <QtConcurrent>

namespace lukija {

Reader::Reader(Library *library, QObject *parent)
    : QObject(parent)
    , m_library(library)
{
    connect(&m_watcher, &QFutureWatcher<LoadResult>::finished, this, &Reader::onLoaded);

    // Save a few seconds after the position changes, and at once on pause.
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(3000);
    connect(&m_saveTimer, &QTimer::timeout, this, &Reader::savePosition);
    connect(&m_player, &PlaybackController::currentSentenceChanged, &m_saveTimer,
            static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(&m_player, &PlaybackController::playingChanged, this, [this]() {
        if (!m_player.isPlaying())
            savePosition();
    });
}

Reader::~Reader()
{
    savePosition();
    m_watcher.waitForFinished();
}

QString Reader::title() const
{
    return m_document ? m_document->title : QString();
}

int Reader::pageCount() const
{
    return m_document ? int(m_document->pages.size()) : 0;
}

void Reader::open(const QString &pathOrUrl)
{
    const QString path = pathOrUrl.startsWith(QLatin1String("file:"))
                             ? QUrl(pathOrUrl).toLocalFile()
                             : pathOrUrl;
    if (path.isEmpty())
        return;
    if (path == m_path && (m_status == Ready || m_status == Loading))
        return;

    close();
    m_path = path;
    emit documentChanged();
    if (m_watcher.isRunning())
        m_pendingPath = path;
    else
        startLoading(path);
}

void Reader::close()
{
    savePosition();
    m_player.setDocument(nullptr);
    m_sentences.setDocument(nullptr);
    m_document.reset();
    m_documentId = -1;
    m_path.clear();
    m_pendingPath.clear();
    emit documentChanged();
    setStatus(Empty);
}

QString Reader::sentenceText(int index) const
{
    if (!m_document || index < 0 || index >= int(m_document->sentences.size()))
        return QString();
    return m_document->sentences[size_t(index)].spoken;
}

void Reader::savePosition()
{
    m_saveTimer.stop();
    if (!m_document || m_documentId < 0)
        return;
    m_library->savePosition(m_documentId, Library::positionOf(*m_document, m_player.currentSentence()));
}

Reader::LoadResult Reader::load(const QString &path)
{
    LoadResult result;
    result.path = path;
    result.hash = Library::fileHash(path);
    if (result.hash.isEmpty()) {
        result.error = QFileInfo(path).exists() ? tr("The file can't be read")
                                                : tr("File not found");
        return result;
    }

    auto document = std::make_shared<ReadingDocument>();
    if (!Library::loadCached(result.hash, document.get())) {
        if (!Pipeline::readPdf(path, document.get(), &result.error))
            return result;
        if (document->sentences.empty()) {
            result.error = tr("No readable text found in this PDF");
            return result;
        }
        Library::saveCached(result.hash, *document);
    }
    result.document = document;
    return result;
}

void Reader::startLoading(const QString &path)
{
    setStatus(Loading);
    m_watcher.setFuture(QtConcurrent::run(&Reader::load, path));
}

void Reader::onLoaded()
{
    const LoadResult result = m_watcher.result();
    if (!m_pendingPath.isEmpty()) {
        const QString next = m_pendingPath;
        m_pendingPath.clear();
        startLoading(next);
        return;
    }
    if (result.path != m_path)
        return;  // closed while loading
    if (!result.document) {
        setStatus(Error, result.error);
        return;
    }

    m_document = result.document;
    m_documentId = m_library->addDocument(result.hash, result.path, *m_document);
    Position position;
    const int start = m_library->loadPosition(m_documentId, &position)
                          ? Library::relocate(*m_document, position)
                          : 0;
    m_sentences.setDocument(m_document);
    m_player.setDocument(m_document, start);
    emit documentChanged();
    setStatus(Ready);
    emit opened();
}

void Reader::setStatus(Status status, const QString &error)
{
    if (m_status == status && m_errorString == error)
        return;
    m_status = status;
    m_errorString = error;
    emit statusChanged();
}

} // namespace lukija
