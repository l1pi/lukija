#pragma once

#include <QString>

namespace lukija {

// Tells Finnish from English by common function words and the letters ä/ö.
class LanguageDetector
{
public:
    // "fi", "en", or an empty string when the text is too short or unclear.
    static QString detect(const QString &text);
};

} // namespace lukija
