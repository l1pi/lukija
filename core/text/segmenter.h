#pragma once

#include "text/abbreviations.h"

#include <QString>

#include <vector>

namespace lukija {

struct TextSpan {
    int start = 0;
    int length = 0;
};

// Splits text into sentences: Unicode sentence boundaries (UAX #29), re-joined
// after abbreviations and initials. Very long sentences are split further at
// ";" or "," so synthesis of a single chunk stays quick.
class Segmenter
{
public:
    explicit Segmenter(int maxLength = 300);

    std::vector<TextSpan> split(const QString &text) const;

private:
    bool continuesSentence(const QString &text, TextSpan previous, TextSpan next) const;
    std::vector<TextSpan> splitLong(const QString &text, TextSpan span) const;
    static TextSpan trimmed(const QString &text, TextSpan span);
    static bool endsWithTerminal(const QString &text, TextSpan span);

    Abbreviations m_abbreviations;
    int m_maxLength;
};

} // namespace lukija
