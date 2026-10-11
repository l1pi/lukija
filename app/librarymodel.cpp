#include "librarymodel.h"

#include <QDateTime>
#include <QFileInfo>

namespace lukija {

LibraryModel::LibraryModel(Library *library, QObject *parent)
    : QAbstractListModel(parent)
    , m_library(library)
{
    refresh();
}

int LibraryModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_documents.size();
}

QVariant LibraryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_documents.size())
        return QVariant();
    const DocumentRecord &document = m_documents.at(index.row());
    switch (role) {
    case DocumentIdRole:
        return document.id;
    case TitleRole:
        return document.title;
    case PathRole:
        return document.path;
    case ProgressRole:
        return document.sentenceCount > 1
                   ? qreal(document.sentenceIndex) / (document.sentenceCount - 1)
                   : 0.0;
    case PageCountRole:
        return document.pageCount;
    case LastOpenedRole:
        return QDateTime::fromMSecsSinceEpoch(document.lastOpenedAt);
    case AvailableRole:
        return QFileInfo(document.path).isFile();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> LibraryModel::roleNames() const
{
    return {
        {DocumentIdRole, "documentId"},
        {TitleRole, "title"},
        {PathRole, "path"},
        {ProgressRole, "progress"},
        {PageCountRole, "pageCount"},
        {LastOpenedRole, "lastOpened"},
        {AvailableRole, "available"},
    };
}

void LibraryModel::refresh()
{
    const int oldCount = m_documents.size();
    beginResetModel();
    m_documents = m_library->recentDocuments();
    endResetModel();
    if (m_documents.size() != oldCount)
        emit countChanged();
}

void LibraryModel::remove(int documentId)
{
    for (int row = 0; row < m_documents.size(); ++row) {
        if (m_documents.at(row).id != documentId)
            continue;
        beginRemoveRows(QModelIndex(), row, row);
        m_library->removeDocument(documentId);
        m_documents.removeAt(row);
        endRemoveRows();
        emit countChanged();
        return;
    }
}

} // namespace lukija
