#include "text/normalizer.h"

#include <QTextBoundaryFinder>

namespace lukija {

void Normalizer::normalize(MappedText &text)
{
    removeInvisible(text);
    applyNfkc(text);
    collapseWhitespace(text);
}

void Normalizer::applyNfkc(MappedText &text)
{
    // Normalise one grapheme cluster at a time, so each replacement keeps the
    // source of the characters it replaces.
    struct Cluster { int start; int length; QString normalized; };
    std::vector<Cluster> changes;

    const QString &s = text.text();
    QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, s);
    int start = 0;
    while (finder.toNextBoundary() != -1) {
        const int end = finder.position();
        bool ascii = true;
        for (int i = start; i < end && ascii; ++i)
            ascii = s.at(i).unicode() < 0x80;
        if (!ascii) {
            const QString cluster = s.mid(start, end - start);
            const QString normalized = cluster.normalized(QString::NormalizationForm_KC);
            if (normalized != cluster)
                changes.push_back({start, end - start, normalized});
        }
        start = end;
    }

    for (auto it = changes.rbegin(); it != changes.rend(); ++it)
        text.replace(it->start, it->length, it->normalized);
}

void Normalizer::removeInvisible(MappedText &text)
{
    for (int i = text.size() - 1; i >= 0; --i) {
        switch (text.text().at(i).unicode()) {
        case 0x00AD: // soft hyphen
        case 0x200B: // zero width space
        case 0x200C: // zero width non-joiner
        case 0x200D: // zero width joiner
        case 0x2060: // word joiner
        case 0xFEFF: // zero width no-break space
            text.remove(i, 1);
            break;
        default:
            break;
        }
    }
}

void Normalizer::collapseWhitespace(MappedText &text)
{
    // Every whitespace run becomes one space; leading and trailing runs go.
    int i = text.size();
    while (i > 0) {
        if (!text.text().at(i - 1).isSpace()) {
            --i;
            continue;
        }
        int start = i - 1;
        while (start > 0 && text.text().at(start - 1).isSpace())
            --start;
        const int length = i - start;
        if (start == 0 || i == text.size())
            text.remove(start, length);
        else if (length > 1 || text.text().at(start) != QLatin1Char(' '))
            text.replace(start, length, QStringLiteral(" "));
        i = start;
    }
}

} // namespace lukija
