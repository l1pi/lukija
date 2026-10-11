#include "model/document.h"

namespace lukija {

const char *blockKindName(BlockKind kind)
{
    switch (kind) {
    case BlockKind::Body: return "Body";
    case BlockKind::Heading: return "Heading";
    case BlockKind::Header: return "Header";
    case BlockKind::Footer: return "Footer";
    case BlockKind::PageNumber: return "PageNumber";
    case BlockKind::Footnote: return "Footnote";
    case BlockKind::Caption: return "Caption";
    case BlockKind::Table: return "Table";
    case BlockKind::Equation: return "Equation";
    case BlockKind::FrontMatter: return "FrontMatter";
    case BlockKind::References: return "References";
    }
    return "?";
}

} // namespace lukija
