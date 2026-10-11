#pragma once

#include "text/mappedtext.h"

namespace lukija {

// Character-level clean-up before segmentation: Unicode NFKC (so ligatures
// like "ﬁ" become "fi"), soft hyphens and zero-width characters removed,
// whitespace collapsed and trimmed.
class Normalizer
{
public:
    static void normalize(MappedText &text);

private:
    static void applyNfkc(MappedText &text);
    static void removeInvisible(MappedText &text);
    static void collapseWhitespace(MappedText &text);
};

} // namespace lukija
