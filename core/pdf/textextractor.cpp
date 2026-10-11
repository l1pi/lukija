#include "pdf/textextractor.h"

#include <poppler-qt5.h>

#include <QtMath>

namespace lukija {

PageText TextExtractor::extract(const Poppler::Page &page)
{
    PageText result;
    result.size = page.pageSizeF();

    const QList<Poppler::TextBox *> boxes = page.textList();
    std::vector<const Poppler::TextBox *> kept;
    kept.reserve(boxes.size());
    for (const Poppler::TextBox *box : boxes) {
        if (!box->text().trimmed().isEmpty() && !isVertical(*box))
            kept.push_back(box);
    }

    result.words.reserve(kept.size());
    for (size_t i = 0; i < kept.size(); ++i) {
        const Poppler::TextBox *box = kept[i];
        Word word;
        word.text = box->text();
        word.box = box->boundingBox();
        word.spaceAfter = box->hasSpaceAfter();
        // Poppler links the words of one line through nextWord().
        word.endsLine = i + 1 == kept.size() || box->nextWord() != kept[i + 1];
        result.words.push_back(word);
    }

    qDeleteAll(boxes);
    return result;
}

bool TextExtractor::isVertical(const Poppler::TextBox &box)
{
    const int length = box.text().length();
    if (length < 2)
        return false;
    const QPointF first = box.charBoundingBox(0).center();
    const QPointF last = box.charBoundingBox(length - 1).center();
    return qAbs(last.y() - first.y()) > qAbs(last.x() - first.x());
}

} // namespace lukija
