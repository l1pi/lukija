#pragma once

#include "model/document.h"

#include <QSizeF>
#include <QString>

#include <memory>

namespace Poppler {
class Document;
}

namespace lukija {

class PdfDocument
{
public:
    enum class Error { None, NotFound, Invalid, Locked };

    static std::unique_ptr<PdfDocument> open(const QString &path, Error *error = nullptr);
    static QString errorString(Error error);

    ~PdfDocument();

    int pageCount() const;
    QString title() const;   // from the document info dictionary; may be empty
    PageText pageText(int page) const;
    std::vector<PageText> allPageText() const;

    Poppler::Document *poppler() const { return m_doc.get(); }

private:
    explicit PdfDocument(std::unique_ptr<Poppler::Document> doc);

    std::unique_ptr<Poppler::Document> m_doc;
};

} // namespace lukija
