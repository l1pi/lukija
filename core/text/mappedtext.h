#pragma once

#include <QString>

#include <vector>

namespace lukija {

// Text with a source index for every UTF-16 code unit, so that edits keep
// track of which source words each character came from.
class MappedText
{
public:
    static constexpr int NoSource = -1;

    void append(const QString &text, int source = NoSource);

    // The replacement inherits the source of the first replaced unit. A pure
    // insertion (length 0) gets NoSource.
    void replace(int position, int length, const QString &replacement);
    void remove(int position, int length) { replace(position, length, QString()); }

    const QString &text() const { return m_text; }
    int size() const { return m_text.size(); }
    bool isEmpty() const { return m_text.isEmpty(); }
    int source(int position) const { return m_sources[size_t(position)]; }

    // Distinct sources in [position, position + length), in order of first
    // appearance, without NoSource.
    std::vector<int> sources(int position, int length) const;

private:
    QString m_text;
    std::vector<int> m_sources;
};

} // namespace lukija
