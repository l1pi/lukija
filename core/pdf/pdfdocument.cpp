#include "pdf/pdfdocument.h"

#include "pdf/textextractor.h"

#include <QFileInfo>

#include <poppler-qt5.h>

namespace lukija {

std::unique_ptr<PdfDocument> PdfDocument::open(const QString &path, Error *error)
{
    auto fail = [error](Error e) {
        if (error)
            *error = e;
        return nullptr;
    };

    if (!QFileInfo(path).isFile())
        return fail(Error::NotFound);

    std::unique_ptr<Poppler::Document> doc(Poppler::Document::load(path));
    if (!doc)
        return fail(Error::Invalid);
    if (doc->isLocked())
        return fail(Error::Locked);

    if (error)
        *error = Error::None;
    return std::unique_ptr<PdfDocument>(new PdfDocument(std::move(doc)));
}

QString PdfDocument::errorString(Error error)
{
    switch (error) {
    case Error::None: return QString();
    case Error::NotFound: return QStringLiteral("File not found");
    case Error::Invalid: return QStringLiteral("Not a readable PDF file");
    case Error::Locked: return QStringLiteral("The PDF is password protected");
    }
    return QString();
}

PdfDocument::PdfDocument(std::unique_ptr<Poppler::Document> doc)
    : m_doc(std::move(doc))
{
}

PdfDocument::~PdfDocument() = default;

int PdfDocument::pageCount() const
{
    return m_doc->numPages();
}

QString PdfDocument::title() const
{
    return m_doc->info(QStringLiteral("Title")).trimmed();
}

PageText PdfDocument::pageText(int page) const
{
    std::unique_ptr<Poppler::Page> p(m_doc->page(page));
    if (!p)
        return PageText();
    return TextExtractor::extract(*p);
}

std::vector<PageText> PdfDocument::allPageText() const
{
    std::vector<PageText> pages;
    pages.reserve(pageCount());
    for (int i = 0; i < pageCount(); ++i)
        pages.push_back(pageText(i));
    return pages;
}

} // namespace lukija
