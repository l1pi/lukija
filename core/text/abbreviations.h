#pragma once

#include <QSet>
#include <QString>

namespace lukija {

// Abbreviations that end with a period but don't end a sentence, in English
// and Finnish. Both lists always apply: Finnish texts quote English sources
// and vice versa. Expansions for speech ("esim." -> "esimerkiksi") come later.
class Abbreviations
{
public:
    Abbreviations();

    // token: the last whitespace-separated token before a candidate sentence
    // break, e.g. "(e.g." or "Dr."; leading brackets and quotes are ignored.
    // True for abbreviations that never end a sentence, and for initials.
    bool preventsSentenceBreak(const QString &token) const;

    // True for any known abbreviation, including those that can end a
    // sentence ("et al.", "etc.", "jne.").
    bool isAbbreviation(const QString &token) const;

private:
    static QString normalizedToken(const QString &token);

    QSet<QString> m_nonTerminal;
    QSet<QString> m_terminal;
};

} // namespace lukija
