#include "text/dehyphenator.h"

namespace lukija {

namespace {

constexpr ushort HyphenMinus = 0x002D;
constexpr ushort SoftHyphen = 0x00AD;
constexpr ushort Hyphen = 0x2010;

bool isHyphen(QChar c)
{
    return c.unicode() == HyphenMinus || c.unicode() == SoftHyphen || c.unicode() == Hyphen;
}

bool isDash(QChar c)
{
    return c.unicode() == 0x2013 || c.unicode() == 0x2014; // en dash, em dash
}

bool isVowel(QChar c)
{
    return QStringLiteral("aeiouyäöå").contains(c.toLower());
}

// Trailing run of letters, e.g. "(infor" -> "infor".
QString trailingLetters(const QString &s)
{
    int start = s.size();
    while (start > 0 && s.at(start - 1).isLetter())
        --start;
    return s.mid(start);
}

// Leading run of letters, e.g. "mation," -> "mation".
QString leadingLetters(const QString &s)
{
    int end = 0;
    while (end < s.size() && s.at(end).isLetter())
        ++end;
    return s.left(end);
}

} // namespace

void Dehyphenator::addToVocabulary(const QString &word)
{
    const QString c = core(word);
    if (!c.isEmpty())
        m_vocabulary.insert(c);
}

bool Dehyphenator::endsWithHyphen(const QString &word)
{
    return !word.isEmpty() && isHyphen(word.at(word.size() - 1));
}

Dehyphenator::Decision Dehyphenator::decide(const QString &lineEnd, const QString &nextStart) const
{
    Decision decision;
    if (lineEnd.isEmpty() || nextStart.isEmpty())
        return decision;

    const QChar last = lineEnd.at(lineEnd.size() - 1);
    if (isDash(last) || last == QLatin1Char('/')) {
        decision.join = Join::Attach;
        return decision;
    }
    if (!isHyphen(last))
        return decision;

    if (last.unicode() == SoftHyphen) {
        decision.join = Join::RemoveHyphen;
        return decision;
    }

    // "linja-" + "-auto": the hyphen belongs to the word.
    if (isHyphen(nextStart.at(0))) {
        decision.join = Join::Attach;
        decision.dropLeadingHyphen = true;
        return decision;
    }

    decision.join = Join::Attach;

    const QString lastWord = lineEnd.left(lineEnd.size() - 1);
    const QString stem = trailingLetters(lastWord);
    const QString head = leadingLetters(nextStart);
    if (stem.isEmpty() || head.isEmpty())
        return decision;                   // "1990-" + "2000", "COVID-" + "19"
    if (!head.at(0).isLower())
        return decision;                   // "Euro-" + "Atlantic"
    if (stem.at(stem.size() - 1).isUpper())
        return decision;                   // "EU-" + "maat"

    const QString stemCore = core(lastWord);
    if (stemCore.contains(QLatin1Char('-')))
        return decision;                   // "state-of-the-" + "art"

    const QString joined = stemCore + head.toLower();
    const QString hyphenated = stemCore + QLatin1Char('-') + head.toLower();
    const bool knownJoined = m_vocabulary.contains(joined);
    const bool knownHyphenated = m_vocabulary.contains(hyphenated);
    if (knownHyphenated && !knownJoined)
        return decision;
    if (knownJoined) {
        decision.join = Join::RemoveHyphen;
        return decision;
    }

    // Identical vowels on both sides only meet at a compound boundary, where
    // Finnish (and English: "re-enter") writes a hyphen: "linja-auto".
    const QChar a = stem.at(stem.size() - 1).toLower();
    const QChar b = head.at(0).toLower();
    if (a == b && isVowel(a))
        return decision;

    decision.join = Join::RemoveHyphen;
    return decision;
}

QString Dehyphenator::core(const QString &word)
{
    int start = 0;
    int end = word.size();
    while (start < end && !word.at(start).isLetterOrNumber())
        ++start;
    while (end > start && !word.at(end - 1).isLetterOrNumber())
        --end;
    return word.mid(start, end - start).toLower();
}

} // namespace lukija
