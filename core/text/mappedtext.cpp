#include "text/mappedtext.h"

#include <algorithm>

namespace lukija {

void MappedText::append(const QString &text, int source)
{
    m_text += text;
    m_sources.insert(m_sources.end(), size_t(text.size()), source);
}

void MappedText::replace(int position, int length, const QString &replacement)
{
    Q_ASSERT(position >= 0 && length >= 0 && position + length <= size());

    const int source = length > 0 ? m_sources[size_t(position)] : NoSource;
    m_text.replace(position, length, replacement);
    const auto first = m_sources.begin() + position;
    m_sources.erase(first, first + length);
    m_sources.insert(m_sources.begin() + position, size_t(replacement.size()), source);
}

std::vector<int> MappedText::sources(int position, int length) const
{
    std::vector<int> result;
    for (int i = position; i < position + length; ++i) {
        const int s = m_sources[size_t(i)];
        if (s != NoSource && std::find(result.begin(), result.end(), s) == result.end())
            result.push_back(s);
    }
    return result;
}

} // namespace lukija
