#pragma once

#include "model/document.h"

#include <QList>
#include <QSqlDatabase>
#include <QString>

namespace lukija {

struct DocumentRecord {
    int id = -1;
    QString hash;
    QString path;
    QString title;
    QString language;
    int pageCount = 0;
    int sentenceCount = 0;
    int sentenceIndex = 0;  // saved position
    qint64 addedAt = 0;     // ms since the epoch
    qint64 lastOpenedAt = 0;
};

struct Position {
    int sentenceIndex = 0;
    int page = 0;
    QString anchorText;  // start of the sentence, to find it again after a pipeline change
    int pipelineVersion = PipelineVersion;
};

// The documents the user has opened and where they stopped reading, in
// SQLite, plus a cache of extracted documents. Used from the main thread,
// except for the static cache functions.
class Library
{
public:
    // Opens or creates the database; an empty path means the app data directory.
    explicit Library(const QString &databasePath = QString());
    ~Library();

    Library(const Library &) = delete;
    Library &operator=(const Library &) = delete;

    bool isOpen() const;

    // Adds the document, or updates its path, title and last-opened time.
    int addDocument(const QString &hash, const QString &path, const ReadingDocument &document);
    QList<DocumentRecord> recentDocuments() const;
    DocumentRecord document(int id) const;
    void removeDocument(int id);

    void savePosition(int documentId, const Position &position);
    bool loadPosition(int documentId, Position *position) const;

    // The position as a sentence index in this document: the saved index if
    // the pipeline hasn't changed, otherwise the sentence found by its text.
    static int relocate(const ReadingDocument &document, const Position &position);
    static Position positionOf(const ReadingDocument &document, int sentenceIndex);

    // SHA-1 of the file size and its first and last megabyte: fast for big
    // files, and a moved or renamed file keeps its progress.
    static QString fileHash(const QString &path);

    // Extraction cache in the cache directory (or cacheDirectory if given).
    static bool loadCached(const QString &hash, ReadingDocument *document,
                           const QString &cacheDirectory = QString());
    static void saveCached(const QString &hash, const ReadingDocument &document,
                           const QString &cacheDirectory = QString());

private:
    bool createSchema();

    QString m_connectionName;
    QSqlDatabase m_db;
};

} // namespace lukija
