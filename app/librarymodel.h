#pragma once

#include "library.h"

#include <QAbstractListModel>

namespace lukija {

// Recently opened documents, most recent first, for the library page.
class LibraryModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Role {
        DocumentIdRole = Qt::UserRole + 1,
        TitleRole,
        PathRole,
        ProgressRole,
        PageCountRole,
        LastOpenedRole,
        AvailableRole,
    };

    explicit LibraryModel(Library *library, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void remove(int documentId);

public slots:
    // The document's reading position changed: updates its progress in place,
    // rather than resetting the list under the user.
    void setPosition(int documentId, int sentenceIndex);

signals:
    void countChanged();

private:
    Library *m_library;
    QList<DocumentRecord> m_documents;
};

} // namespace lukija
