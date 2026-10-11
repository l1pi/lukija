#include "layout/layoutanalyzer.h"

#include <QHash>
#include <QtMath>

#include <algorithm>

namespace lukija {

namespace {

qreal median(std::vector<qreal> values)
{
    if (values.empty())
        return 0;
    const auto middle = values.begin() + values.size() / 2;
    std::nth_element(values.begin(), middle, values.end());
    return *middle;
}

bool overlapsVertically(const QRectF &a, const QRectF &b)
{
    return a.top() < b.bottom() && b.top() < a.bottom();
}

bool overlapsHorizontally(const QRectF &a, const QRectF &b)
{
    return a.left() < b.right() && b.left() < a.right();
}

int textLength(const Line &line, const PageText &page)
{
    int length = 0;
    for (int w : line.words)
        length += page.words[w].text.length() + 1;
    return length;
}

} // namespace

std::vector<Block> LayoutAnalyzer::analyze(const std::vector<PageText> &pages) const
{
    std::vector<std::vector<Line>> lines;
    lines.reserve(pages.size());
    for (const PageText &page : pages)
        lines.push_back(buildLines(page));

    const Stats stats = documentStats(pages, lines);

    std::vector<Block> blocks;
    for (size_t p = 0; p < pages.size(); ++p) {
        Block block;
        auto flush = [&]() {
            if (block.lines.empty())
                return;
            block.kind = classify(block, pages[p], stats);
            blocks.push_back(std::move(block));
            block = Block();
        };

        for (const Line &line : lines[p]) {
            if (!block.lines.empty() && startsNewBlock(block, block.lines.back(), line, stats))
                flush();
            if (block.lines.empty()) {
                block.page = int(p);
                block.box = line.box;
                block.fontSize = line.fontSize;
            } else {
                block.box = block.box.united(line.box);
            }
            block.lines.push_back(line);
        }
        flush();
    }
    return blocks;
}

std::vector<Line> LayoutAnalyzer::buildLines(const PageText &page)
{
    std::vector<Line> lines;
    Line line;
    std::vector<qreal> heights;

    auto flush = [&]() {
        if (line.words.empty())
            return;
        line.fontSize = median(heights);
        lines.push_back(std::move(line));
        line = Line();
        heights.clear();
    };

    for (size_t i = 0; i < page.words.size(); ++i) {
        const Word &word = page.words[i];
        // Poppler's line links are the primary signal; this guards against
        // words that don't share the line's vertical extent.
        if (!line.words.empty() && !overlapsVertically(line.box, word.box))
            flush();

        line.box = line.words.empty() ? word.box : line.box.united(word.box);
        line.words.push_back(int(i));
        heights.push_back(word.box.height());

        if (word.endsLine)
            flush();
    }
    flush();
    return lines;
}

LayoutAnalyzer::Stats LayoutAnalyzer::documentStats(const std::vector<PageText> &pages,
                                                    const std::vector<std::vector<Line>> &lines)
{
    Stats stats;

    // Font sizes rounded to half a point, weighted by the amount of text.
    QHash<int, int> sizeWeights;
    for (size_t p = 0; p < pages.size(); ++p) {
        for (const Line &line : lines[p])
            sizeWeights[qRound(line.fontSize * 2)] += textLength(line, pages[p]);
    }
    int bestWeight = 0;
    for (auto it = sizeWeights.constBegin(); it != sizeWeights.constEnd(); ++it) {
        if (it.value() > bestWeight) {
            bestWeight = it.value();
            stats.bodyFontSize = it.key() / 2.0;
        }
    }

    std::vector<qreal> gaps;
    for (const std::vector<Line> &pageLines : lines) {
        for (size_t i = 1; i < pageLines.size(); ++i) {
            const Line &a = pageLines[i - 1];
            const Line &b = pageLines[i];
            const qreal gap = b.box.top() - a.box.bottom();
            const bool bodySized = qAbs(a.fontSize - stats.bodyFontSize) < 0.1 * stats.bodyFontSize
                                   && qAbs(b.fontSize - stats.bodyFontSize) < 0.1 * stats.bodyFontSize;
            if (bodySized && gap >= -0.5 * a.fontSize && gap < 2 * a.fontSize)
                gaps.push_back(gap);
        }
    }
    stats.lineGap = median(gaps);
    return stats;
}

bool LayoutAnalyzer::startsNewBlock(const Block &block, const Line &previous, const Line &line,
                                    const Stats &stats)
{
    const qreal fontSize = qMax(line.fontSize, qreal(1));

    // Font size change: heading to body text and back.
    if (qAbs(line.fontSize - block.fontSize) > 0.15 * block.fontSize)
        return true;

    // Moved up the page (column change), or more space than normal line spacing.
    const qreal gap = line.box.top() - previous.box.bottom();
    if (gap < -0.5 * fontSize)
        return true;
    if (gap > stats.lineGap + 0.4 * fontSize)
        return true;

    if (!overlapsHorizontally(block.box, line.box))
        return true;

    // First-line indent after a line that was flush with the block's left edge.
    const qreal indent = line.box.left() - previous.box.left();
    const bool previousFlush = qAbs(previous.box.left() - block.box.left()) < 0.5 * fontSize;
    if (previousFlush && indent > 0.8 * fontSize && indent < 6 * fontSize)
        return true;

    return false;
}

BlockKind LayoutAnalyzer::classify(const Block &block, const PageText &page, const Stats &stats)
{
    int length = 0;
    for (const Line &line : block.lines)
        length += textLength(line, page);

    if (block.fontSize > 1.15 * stats.bodyFontSize && block.lines.size() <= 4 && length < 200)
        return BlockKind::Heading;
    return BlockKind::Body;
}

} // namespace lukija
