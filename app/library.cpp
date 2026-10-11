#include "library.h"

#include "model/serialization.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>
#include <QVariant>

namespace lukija {

namespace {

constexpr int SchemaVersion = 1;
constexpr int AnchorLength = 60;
constexpr qint64 HashChunk = 1024 * 1024;

QString defaultDatabasePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/library.sqlite");
}

QString cacheFile(const QString &hash, const QString &cacheDirectory)
{
    const QString directory = cacheDirectory.isEmpty()
                                  ? QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                                  : cacheDirectory;
    return directory + QLatin1Char('/') + hash + QStringLiteral(".bin");
}

qint64 now()
{
    return QDateTime::currentMSecsSinceEpoch();
}

bool exec(QSqlQuery &query)
{
    if (query.exec())
        return true;
    qWarning("Library: %s", qPrintable(query.lastError().text()));
    return false;
}

} // namespace

Library::Library(const QString &databasePath)
    : m_connectionName(QStringLiteral("library-") + QUuid::createUuid().toString())
{
    const QString path = databasePath.isEmpty() ? defaultDatabasePath() : databasePath;
    QDir().mkpath(QFileInfo(path).absolutePath());

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_db.setDatabaseName(path);
    if (!m_db.open() || !createSchema()) {
        qWarning("Library: cannot open %s: %s", qPrintable(path), qPrintable(m_db.lastError().text()));
        m_db.close();
    }
}

Library::~Library()
{
    m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool Library::isOpen() const
{
    return m_db.isOpen();
}

bool Library::createSchema()
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("PRAGMA user_version"));
    if (!exec(query) || !query.next())
        return false;
    if (query.value(0).toInt() >= SchemaVersion)
        return true;

    const QStringList statements = {
        QStringLiteral("CREATE TABLE IF NOT EXISTS documents ("
                       " id INTEGER PRIMARY KEY, file_hash TEXT UNIQUE, path TEXT, title TEXT,"
                       " page_count INTEGER, sentence_count INTEGER, language TEXT, voice_id TEXT,"
                       " settings_json TEXT, added_at INTEGER, last_opened_at INTEGER)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS positions ("
                       " document_id INTEGER PRIMARY KEY REFERENCES documents(id) ON DELETE CASCADE,"
                       " sentence_index INTEGER, page INTEGER, anchor_text TEXT,"
                       " pipeline_version INTEGER, updated_at INTEGER)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS bookmarks ("
                       " id INTEGER PRIMARY KEY,"
                       " document_id INTEGER REFERENCES documents(id) ON DELETE CASCADE,"
                       " sentence_index INTEGER, page INTEGER, anchor_text TEXT, label TEXT,"
                       " created_at INTEGER)"),
        QStringLiteral("PRAGMA user_version = %1").arg(SchemaVersion),
    };
    for (const QString &statement : statements) {
        query.prepare(statement);
        if (!exec(query))
            return false;
    }
    return true;
}

int Library::addDocument(const QString &hash, const QString &path, const ReadingDocument &document)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT id FROM documents WHERE file_hash = ?"));
    query.addBindValue(hash);
    if (!exec(query))
        return -1;

    if (query.next()) {
        const int id = query.value(0).toInt();
        QSqlQuery update(m_db);
        update.prepare(QStringLiteral("UPDATE documents SET path = ?, title = ?, page_count = ?,"
                                      " sentence_count = ?, language = ?, last_opened_at = ?"
                                      " WHERE id = ?"));
        update.addBindValue(path);
        update.addBindValue(document.title);
        update.addBindValue(int(document.pages.size()));
        update.addBindValue(int(document.sentences.size()));
        update.addBindValue(document.lang);
        update.addBindValue(now());
        update.addBindValue(id);
        exec(update);
        return id;
    }

    QSqlQuery insert(m_db);
    insert.prepare(QStringLiteral("INSERT INTO documents (file_hash, path, title, page_count,"
                                  " sentence_count, language, added_at, last_opened_at)"
                                  " VALUES (?, ?, ?, ?, ?, ?, ?, ?)"));
    insert.addBindValue(hash);
    insert.addBindValue(path);
    insert.addBindValue(document.title);
    insert.addBindValue(int(document.pages.size()));
    insert.addBindValue(int(document.sentences.size()));
    insert.addBindValue(document.lang);
    insert.addBindValue(now());
    insert.addBindValue(now());
    if (!exec(insert))
        return -1;
    return insert.lastInsertId().toInt();
}

QList<DocumentRecord> Library::recentDocuments() const
{
    QList<DocumentRecord> result;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT d.id, d.file_hash, d.path, d.title, d.language, d.page_count, d.sentence_count,"
        " d.added_at, d.last_opened_at, COALESCE(p.sentence_index, 0)"
        " FROM documents d LEFT JOIN positions p ON p.document_id = d.id"
        " ORDER BY d.last_opened_at DESC"));
    if (!exec(query))
        return result;
    while (query.next()) {
        DocumentRecord record;
        record.id = query.value(0).toInt();
        record.hash = query.value(1).toString();
        record.path = query.value(2).toString();
        record.title = query.value(3).toString();
        record.language = query.value(4).toString();
        record.pageCount = query.value(5).toInt();
        record.sentenceCount = query.value(6).toInt();
        record.addedAt = query.value(7).toLongLong();
        record.lastOpenedAt = query.value(8).toLongLong();
        record.sentenceIndex = query.value(9).toInt();
        result.append(record);
    }
    return result;
}

DocumentRecord Library::document(int id) const
{
    const QList<DocumentRecord> documents = recentDocuments();
    for (const DocumentRecord &record : documents) {
        if (record.id == id)
            return record;
    }
    return DocumentRecord();
}

void Library::removeDocument(int id)
{
    const QStringList statements = {
        QStringLiteral("DELETE FROM positions WHERE document_id = ?"),
        QStringLiteral("DELETE FROM bookmarks WHERE document_id = ?"),
        QStringLiteral("DELETE FROM documents WHERE id = ?"),
    };
    for (const QString &statement : statements) {
        QSqlQuery query(m_db);
        query.prepare(statement);
        query.addBindValue(id);
        exec(query);
    }
}

void Library::savePosition(int documentId, const Position &position)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("INSERT OR REPLACE INTO positions (document_id, sentence_index,"
                                 " page, anchor_text, pipeline_version, updated_at)"
                                 " VALUES (?, ?, ?, ?, ?, ?)"));
    query.addBindValue(documentId);
    query.addBindValue(position.sentenceIndex);
    query.addBindValue(position.page);
    query.addBindValue(position.anchorText);
    query.addBindValue(position.pipelineVersion);
    query.addBindValue(now());
    exec(query);
}

bool Library::loadPosition(int documentId, Position *position) const
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT sentence_index, page, anchor_text, pipeline_version"
                                 " FROM positions WHERE document_id = ?"));
    query.addBindValue(documentId);
    if (!exec(query) || !query.next())
        return false;
    position->sentenceIndex = query.value(0).toInt();
    position->page = query.value(1).toInt();
    position->anchorText = query.value(2).toString();
    position->pipelineVersion = query.value(3).toInt();
    return true;
}

Position Library::positionOf(const ReadingDocument &document, int sentenceIndex)
{
    Position position;
    if (sentenceIndex < 0 || sentenceIndex >= int(document.sentences.size()))
        return position;
    const Sentence &sentence = document.sentences[size_t(sentenceIndex)];
    position.sentenceIndex = sentenceIndex;
    position.page = sentence.page;
    position.anchorText = sentence.spoken.left(AnchorLength);
    position.pipelineVersion = document.pipelineVersion;
    return position;
}

int Library::relocate(const ReadingDocument &document, const Position &position)
{
    const int count = int(document.sentences.size());
    if (count == 0)
        return 0;
    if (position.pipelineVersion == document.pipelineVersion && position.sentenceIndex >= 0
        && position.sentenceIndex < count)
        return position.sentenceIndex;

    // The sentence that starts like the saved one, nearest to the saved page.
    int best = -1;
    int bestDistance = 0;
    if (!position.anchorText.isEmpty()) {
        for (int i = 0; i < count; ++i) {
            const Sentence &sentence = document.sentences[size_t(i)];
            if (!sentence.spoken.startsWith(position.anchorText)
                && !position.anchorText.startsWith(sentence.spoken))
                continue;
            const int distance = qAbs(sentence.page - position.page);
            if (best < 0 || distance < bestDistance) {
                best = i;
                bestDistance = distance;
            }
        }
    }
    if (best >= 0)
        return best;

    // Otherwise the first sentence on the saved page.
    for (int i = 0; i < count; ++i) {
        if (document.sentences[size_t(i)].page >= position.page)
            return i;
    }
    return 0;
}

QString Library::fileHash(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QString();

    const qint64 size = file.size();
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(QByteArray::number(size));
    hash.addData(file.read(HashChunk));
    if (size > HashChunk) {
        file.seek(qMax(HashChunk, size - HashChunk));
        hash.addData(file.read(HashChunk));
    }
    return QString::fromLatin1(hash.result().toHex());
}

bool Library::loadCached(const QString &hash, ReadingDocument *document, const QString &cacheDirectory)
{
    QFile file(cacheFile(hash, cacheDirectory));
    if (hash.isEmpty() || !file.open(QIODevice::ReadOnly))
        return false;
    return deserialize(file.readAll(), document);
}

void Library::saveCached(const QString &hash, const ReadingDocument &document,
                         const QString &cacheDirectory)
{
    if (hash.isEmpty())
        return;
    const QString path = cacheFile(hash, cacheDirectory);
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(serialize(document));
        file.commit();
    }
}

} // namespace lukija
