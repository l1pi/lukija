#pragma once

#include "model/document.h"

namespace lukija {

// Groups words into lines and lines into blocks (paragraph-sized units), and
// classifies blocks.
//
// Single-column for now: it keeps Poppler's reading order, which already
// handles many two-column layouts. Own column detection, headers/footers,
// footnotes etc. come later (see PLAN.md, phase 3).
class LayoutAnalyzer
{
public:
    std::vector<Block> analyze(const std::vector<PageText> &pages) const;

    // Exposed for tests.
    static std::vector<Line> buildLines(const PageText &page);

private:
    struct Stats {
        qreal bodyFontSize = 0;  // most common line font size, weighted by text length
        qreal lineGap = 0;       // median vertical gap between consecutive body lines
    };

    static Stats documentStats(const std::vector<PageText> &pages,
                               const std::vector<std::vector<Line>> &lines);
    static bool startsNewBlock(const Block &block, const Line &previous, const Line &line,
                               const Stats &stats);
    static BlockKind classify(const Block &block, const PageText &page, const Stats &stats);
};

} // namespace lukija
