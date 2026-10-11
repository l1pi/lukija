#include "text/segmenter.h"

#include <QTextBoundaryFinder>

namespace lukija {

Segmenter::Segmenter(int maxLength)
    : m_maxLength(maxLength)
{
}

std::vector<TextSpan> Segmenter::split(const QString &text) const
{
    std::vector<TextSpan> raw;
    QTextBoundaryFinder finder(QTextBoundaryFinder::Sentence, text);
    int start = 0;
    while (finder.toNextBoundary() != -1) {
        const int end = finder.position();
        if (end > start)
            raw.push_back({start, end - start});
        start = end;
    }

    std::vector<TextSpan> merged;
    for (const TextSpan &span : raw) {
        if (!merged.empty() && continuesSentence(text, merged.back(), span)) {
            merged.back().length += span.length;
            continue;
        }
        merged.push_back(span);
    }

    std::vector<TextSpan> result;
    for (const TextSpan &span : merged) {
        const TextSpan t = trimmed(text, span);
        if (t.length == 0)
            continue;
        for (const TextSpan &part : splitLong(text, t))
            result.push_back(part);
    }
    return result;
}

// Qt's boundary finder breaks in places a reader wouldn't: after "et al.,",
// after abbreviations and initials. Re-join those.
bool Segmenter::continuesSentence(const QString &text, TextSpan previous, TextSpan next) const
{
    const TextSpan p = trimmed(text, previous);
    const TextSpan n = trimmed(text, next);
    if (p.length == 0 || n.length == 0)
        return true;
    if (!endsWithTerminal(text, p))
        return true;

    const int lastSpace = text.lastIndexOf(QLatin1Char(' '), p.start + p.length - 1);
    const int tokenStart = qMax(lastSpace + 1, p.start);
    const QString token = text.mid(tokenStart, p.start + p.length - tokenStart);
    if (m_abbreviations.preventsSentenceBreak(token))
        return true;
    return m_abbreviations.isAbbreviation(token) && !text.at(n.start).isUpper();
}

bool Segmenter::endsWithTerminal(const QString &text, TextSpan span)
{
    int i = span.start + span.length - 1;
    while (i >= span.start && QStringLiteral("\"'”’»)]").contains(text.at(i)))
        --i;
    return i >= span.start && QStringLiteral(".!?…").contains(text.at(i));
}

std::vector<TextSpan> Segmenter::splitLong(const QString &text, TextSpan span) const
{
    std::vector<TextSpan> parts;
    while (span.length > m_maxLength) {
        // Latest good break in the second half of the allowed length.
        const int windowStart = span.start + m_maxLength / 2;
        const int windowEnd = span.start + m_maxLength;
        int cut = -1;
        for (const QString &separator : {QStringLiteral("; "), QStringLiteral(", "),
                                         QStringLiteral(" – "), QStringLiteral(" — "),
                                         QStringLiteral(" ")}) {
            const int at = text.lastIndexOf(separator, windowEnd - 1);
            if (at >= windowStart) {
                cut = at + separator.size();
                break;
            }
        }
        if (cut < 0)
            cut = windowEnd; // no space at all: hard cut

        const TextSpan head = trimmed(text, {span.start, cut - span.start});
        if (head.length > 0)
            parts.push_back(head);
        span = trimmed(text, {cut, span.start + span.length - cut});
    }
    if (span.length > 0)
        parts.push_back(span);
    return parts;
}

TextSpan Segmenter::trimmed(const QString &text, TextSpan span)
{
    int start = span.start;
    int end = span.start + span.length;
    while (start < end && text.at(start).isSpace())
        ++start;
    while (end > start && text.at(end - 1).isSpace())
        --end;
    return {start, end - start};
}

} // namespace lukija
