#pragma once

#include "model/document.h"

namespace Poppler {
class Page;
class TextBox;
}

namespace lukija {

class TextExtractor
{
public:
    // Words in Poppler's reading order, with rotated text (typically
    // "Downloaded from ..." watermarks in the margin) dropped.
    static PageText extract(const Poppler::Page &page);

private:
    static bool isVertical(const Poppler::TextBox &box);
};

} // namespace lukija
