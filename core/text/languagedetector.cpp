#include "text/languagedetector.h"

#include <QRegularExpression>
#include <QSet>

namespace lukija {

namespace {

// "on" is left out of both: it's frequent in Finnish and English alike.
const QSet<QString> &finnishWords()
{
    static const QSet<QString> words = {
        QStringLiteral("ja"), QStringLiteral("ei"), QStringLiteral("että"), QStringLiteral("se"),
        QStringLiteral("oli"), QStringLiteral("ovat"), QStringLiteral("tai"), QStringLiteral("kuin"),
        QStringLiteral("myös"), QStringLiteral("joka"), QStringLiteral("sekä"), QStringLiteral("mutta"),
        QStringLiteral("tämä"), QStringLiteral("ole"), QStringLiteral("hän"), QStringLiteral("niin"),
        QStringLiteral("kun"), QStringLiteral("jos"), QStringLiteral("vain"), QStringLiteral("voi"),
        QStringLiteral("mukaan"), QStringLiteral("sen"), QStringLiteral("siitä"), QStringLiteral("nämä"),
        QStringLiteral("eli"), QStringLiteral("jotka"), QStringLiteral("olla"), QStringLiteral("jo"),
        QStringLiteral("kanssa"), QStringLiteral("sitä"), QStringLiteral("hänen"),
    };
    return words;
}

const QSet<QString> &englishWords()
{
    static const QSet<QString> words = {
        QStringLiteral("the"), QStringLiteral("and"), QStringLiteral("of"), QStringLiteral("to"),
        QStringLiteral("in"), QStringLiteral("is"), QStringLiteral("that"), QStringLiteral("for"),
        QStringLiteral("with"), QStringLiteral("as"), QStringLiteral("was"), QStringLiteral("are"),
        QStringLiteral("by"), QStringLiteral("this"), QStringLiteral("be"), QStringLiteral("it"),
        QStringLiteral("from"), QStringLiteral("which"), QStringLiteral("an"), QStringLiteral("or"),
        QStringLiteral("not"), QStringLiteral("we"), QStringLiteral("have"), QStringLiteral("were"),
    };
    return words;
}

} // namespace

QString LanguageDetector::detect(const QString &text)
{
    static const QRegularExpression wordPattern(QStringLiteral("\\p{L}+"));
    int tokens = 0;
    int fi = 0;
    int en = 0;
    QRegularExpressionMatchIterator it = wordPattern.globalMatch(text.toLower());
    while (it.hasNext()) {
        const QString token = it.next().captured();
        ++tokens;
        if (finnishWords().contains(token))
            ++fi;
        else if (englishWords().contains(token))
            ++en;
    }
    if (tokens < 8)
        return QString();

    int letters = 0;
    int finnishLetters = 0;
    for (const QChar c : text) {
        if (!c.isLetter())
            continue;
        ++letters;
        const ushort u = c.toLower().unicode();
        if (u == 0x00E4 || u == 0x00F6) // ä, ö
            ++finnishLetters;
    }
    // ä and ö make up several percent of Finnish text and are rare in English.
    if (letters > 0 && finnishLetters * 100 > letters)
        fi += qMax(2, tokens / 20);

    if (fi > en * 3 / 2 && fi >= 2)
        return QStringLiteral("fi");
    if (en > fi * 3 / 2 && en >= 2)
        return QStringLiteral("en");
    return QString();
}

} // namespace lukija
