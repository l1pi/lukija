#pragma once

#include <QRectF>
#include <QSizeF>
#include <QString>

#include <vector>

namespace lukija {

// Bump whenever a pipeline change can shift sentence boundaries. Cached
// documents and saved reading positions record the version that produced them.
constexpr int PipelineVersion = 1;

struct Word {
    QString text;
    QRectF box;              // PDF points, origin at the top-left corner of the page
    bool spaceAfter = false;
    bool endsLine = false;   // last word of a text line, as Poppler laid it out
};

struct PageText {
    QSizeF size;             // PDF points
    std::vector<Word> words; // in Poppler's reading order
};

struct WordRef {
    int page = -1;
    int word = -1;           // index into PageText::words

    bool operator==(const WordRef &other) const { return page == other.page && word == other.word; }
};

enum class BlockKind {
    Body,
    Heading,
    Header,
    Footer,
    PageNumber,
    Footnote,
    Caption,
    Table,
    Equation,
    FrontMatter,
    References,
};

const char *blockKindName(BlockKind kind);

struct Line {
    QRectF box;
    qreal fontSize = 0;      // median word box height, a proxy for the font size
    std::vector<int> words;  // indices into PageText::words, left to right
};

struct Block {
    int page = -1;
    QRectF box;
    qreal fontSize = 0;
    BlockKind kind = BlockKind::Body;
    std::vector<Line> lines;
};

struct Sentence {
    QString spoken;               // normalised text sent to TTS
    std::vector<WordRef> source;  // words it came from: highlighting, tap-to-read, resume
    int page = -1;
    BlockKind kind = BlockKind::Body;
    QString lang;                 // "fi", "en", or empty when unknown
    bool paragraphEnd = false;
};

struct ReadingDocument {
    QString title;
    QString lang;
    std::vector<PageText> pages;
    std::vector<Block> blocks;        // in reading order
    std::vector<Sentence> sentences;  // in reading order
    int pipelineVersion = PipelineVersion;
};

} // namespace lukija
