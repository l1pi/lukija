#include "sentencemodel.h"

namespace lukija {

SentenceModel::SentenceModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void SentenceModel::setDocument(std::shared_ptr<const ReadingDocument> document)
{
    beginResetModel();
    m_document = std::move(document);
    endResetModel();
}

int SentenceModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || !m_document)
        return 0;
    return int(m_document->sentences.size());
}

QVariant SentenceModel::data(const QModelIndex &index, int role) const
{
    if (!m_document || !index.isValid() || index.row() >= rowCount())
        return QVariant();
    const Sentence &sentence = m_document->sentences[size_t(index.row())];
    switch (role) {
    case TextRole:
        return sentence.spoken;
    case HeadingRole:
        return sentence.kind == BlockKind::Heading;
    case ParagraphEndRole:
        return sentence.paragraphEnd;
    case PageRole:
        return sentence.page + 1;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> SentenceModel::roleNames() const
{
    return {
        {TextRole, "text"},
        {HeadingRole, "heading"},
        {ParagraphEndRole, "paragraphEnd"},
        {PageRole, "page"},
    };
}

} // namespace lukija
