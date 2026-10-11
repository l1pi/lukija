#pragma once

#include "model/document.h"

#include <QAbstractListModel>

#include <memory>

namespace lukija {

// The sentences of the open document, for the reader's text view.
class SentenceModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        TextRole = Qt::UserRole + 1,
        HeadingRole,
        ParagraphEndRole,
        PageRole,
    };

    explicit SentenceModel(QObject *parent = nullptr);

    void setDocument(std::shared_ptr<const ReadingDocument> document);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    std::shared_ptr<const ReadingDocument> m_document;
};

} // namespace lukija
