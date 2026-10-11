#include "text/abbreviations.h"

#include <QStringList>

namespace lukija {

namespace {

// Lower-case. Abbreviations that never end a sentence.
const char *const English[] = {
    "mr.", "mrs.", "ms.", "dr.", "prof.", "st.", "jr.", "sr.", "rev.", "gen.", "col.", "lt.",
    "vs.", "e.g.", "i.e.", "cf.", "viz.", "approx.", "ca.", "c.", "resp.",
    "fig.", "figs.", "eq.", "eqs.", "tab.", "ref.", "refs.", "sec.", "ch.", "chap.", "app.",
    "vol.", "vols.", "no.", "nos.", "p.", "pp.", "ed.", "eds.", "trans.", "repr.",
    "jan.", "feb.", "mar.", "apr.", "jun.", "jul.", "aug.", "sep.", "sept.", "oct.", "nov.", "dec.",
    "dept.", "univ.", "assoc.", "inc.", "ltd.", "co.", "corp.",
};

const char *const Finnish[] = {
    "esim.", "ks.", "kts.", "vrt.", "mm.", "ns.", "nk.", "ts.", "n.", "s.", "ss.", "v.", "vv.",
    "prof.", "dos.", "toim.", "suom.", "huom.", "ao.", "em.", "ko.", "mt.", "os.", "puh.",
    "vt.", "milj.", "mrd.", "tuh.", "vs.", "ed.",
    "tammik.", "helmik.", "maalisk.", "huhtik.", "toukok.", "kesäk.", "heinäk.", "elok.",
    "syysk.", "lokak.", "marrask.", "jouluk.",
};

// Abbreviations that often end a sentence: after them, only a capital letter
// starts a new one.
const char *const Terminal[] = {
    "al.", "etc.", "jne.", "yms.", "ym.", "tms.", "eaa.", "jaa.", "ekr.", "jkr.",
};

} // namespace

Abbreviations::Abbreviations()
{
    for (const char *a : English)
        m_nonTerminal.insert(QString::fromUtf8(a));
    for (const char *a : Finnish)
        m_nonTerminal.insert(QString::fromUtf8(a));
    for (const char *a : Terminal)
        m_terminal.insert(QString::fromUtf8(a));
}

QString Abbreviations::normalizedToken(const QString &token)
{
    int start = 0;
    while (start < token.size() && !token.at(start).isLetter())
        ++start;
    return token.mid(start);
}

bool Abbreviations::preventsSentenceBreak(const QString &token) const
{
    const QString t = normalizedToken(token);
    if (!t.endsWith(QLatin1Char('.')))
        return false;

    // Initials: "J." in "J. R. R. Tolkien".
    if (t.size() == 2 && t.at(0).isUpper())
        return true;

    return m_nonTerminal.contains(t.toLower());
}

bool Abbreviations::isAbbreviation(const QString &token) const
{
    const QString t = normalizedToken(token).toLower();
    return preventsSentenceBreak(token) || m_terminal.contains(t);
}

} // namespace lukija
