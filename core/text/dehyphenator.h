#pragma once

#include <QSet>
#include <QString>

namespace lukija {

// Decides how to join the last word of a line with the first word of the next
// line. A hyphen at a line break is usually hyphenation ("infor-" + "mation"),
// but can be part of the word ("state-of-the-" + "art", "linja-" + "auto").
// The document itself serves as the dictionary: if the word appears elsewhere
// with or without its hyphen, that settles it.
class Dehyphenator
{
public:
    enum class Join {
        Space,        // ordinary line break between words
        Attach,       // keep both parts as they are, no space ("1990–" + "2000")
        RemoveHyphen, // hyphenation: drop the trailing hyphen, no space
    };

    struct Decision {
        Join join = Join::Space;
        bool dropLeadingHyphen = false; // Finnish style repeats the hyphen on the next line
    };

    // Feed every word of the document except hyphenated line-end fragments.
    void addToVocabulary(const QString &word);

    Decision decide(const QString &lineEnd, const QString &nextStart) const;

    static bool endsWithHyphen(const QString &word);

private:
    static QString core(const QString &word);  // lower-case, surrounding punctuation stripped

    QSet<QString> m_vocabulary;
};

} // namespace lukija
