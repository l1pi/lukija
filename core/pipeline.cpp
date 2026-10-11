#include "pipeline.h"

#include "layout/layoutanalyzer.h"
#include "pdf/pdfdocument.h"
#include "text/dehyphenator.h"
#include "text/languagedetector.h"
#include "text/mappedtext.h"
#include "text/normalizer.h"
#include "text/segmenter.h"

#include <QFileInfo>
#include <QHash>

namespace lukija {

namespace {

struct Paragraph {
    BlockKind kind = BlockKind::Body;
    std::vector<int> blocks;      // indices into ReadingDocument::blocks
    MappedText text;
    std::vector<WordRef> refs;    // MappedText sources index into this
};

bool isSpoken(BlockKind kind)
{
    return kind == BlockKind::Body || kind == BlockKind::Heading;
}

const Word &firstWord(const Block &block, const std::vector<PageText> &pages)
{
    return pages[size_t(block.page)].words[size_t(block.lines.front().words.front())];
}

const Word &lastWord(const Block &block, const std::vector<PageText> &pages)
{
    return pages[size_t(block.page)].words[size_t(block.lines.back().words.back())];
}

bool endsSentence(const QString &word)
{
    int i = word.size() - 1;
    while (i >= 0 && QStringLiteral("\"'”’»)]").contains(word.at(i)))
        --i;
    return i >= 0 && QStringLiteral(".!?…:").contains(word.at(i));
}

// A body block continues the previous paragraph when that one stopped
// mid-sentence: at a page break, a column break or around a figure.
bool continues(const Block &previous, const Block &next, const std::vector<PageText> &pages)
{
    const QString &end = lastWord(previous, pages).text;
    const QString &start = firstWord(next, pages).text;
    if (Dehyphenator::endsWithHyphen(end))
        return true;
    if (endsSentence(end))
        return false;
    return start.at(0).isLower();
}

std::vector<Paragraph> groupParagraphs(const std::vector<Block> &blocks,
                                       const std::vector<PageText> &pages)
{
    std::vector<Paragraph> paragraphs;
    for (size_t i = 0; i < blocks.size(); ++i) {
        const Block &block = blocks[i];
        if (!isSpoken(block.kind))
            continue;
        if (block.kind == BlockKind::Body && !paragraphs.empty()
            && paragraphs.back().kind == BlockKind::Body
            && continues(blocks[size_t(paragraphs.back().blocks.back())], block, pages)) {
            paragraphs.back().blocks.push_back(int(i));
            continue;
        }
        Paragraph paragraph;
        paragraph.kind = block.kind;
        paragraph.blocks.push_back(int(i));
        paragraphs.push_back(std::move(paragraph));
    }
    return paragraphs;
}

Dehyphenator buildDehyphenator(const std::vector<PageText> &pages)
{
    Dehyphenator dehyphenator;
    for (const PageText &page : pages) {
        for (const Word &word : page.words) {
            if (!(word.endsLine && Dehyphenator::endsWithHyphen(word.text)))
                dehyphenator.addToVocabulary(word.text);
        }
    }
    return dehyphenator;
}

void assemble(Paragraph &paragraph, const std::vector<Block> &blocks,
              const std::vector<PageText> &pages, const Dehyphenator &dehyphenator)
{
    const Word *previousWord = nullptr;
    for (int b : paragraph.blocks) {
        const Block &block = blocks[size_t(b)];
        const PageText &page = pages[size_t(block.page)];
        for (const Line &line : block.lines) {
            bool dropLeadingHyphen = false;
            if (previousWord) {
                const Dehyphenator::Decision decision =
                    dehyphenator.decide(previousWord->text, page.words[size_t(line.words.front())].text);
                switch (decision.join) {
                case Dehyphenator::Join::Space:
                    paragraph.text.append(QStringLiteral(" "));
                    break;
                case Dehyphenator::Join::Attach:
                    break;
                case Dehyphenator::Join::RemoveHyphen:
                    paragraph.text.remove(paragraph.text.size() - 1, 1);
                    break;
                }
                dropLeadingHyphen = decision.dropLeadingHyphen;
            }

            for (size_t k = 0; k < line.words.size(); ++k) {
                const Word &word = page.words[size_t(line.words[k])];
                QString text = word.text;
                if (k == 0 && dropLeadingHyphen)
                    text.remove(0, 1);
                paragraph.text.append(text, int(paragraph.refs.size()));
                paragraph.refs.push_back({block.page, line.words[k]});
                if (k + 1 < line.words.size() && word.spaceAfter)
                    paragraph.text.append(QStringLiteral(" "));
            }
            previousWord = &page.words[size_t(line.words.back())];
        }
    }
}

} // namespace

ReadingDocument Pipeline::run(std::vector<PageText> pages, const PipelineOptions &options)
{
    ReadingDocument document;
    document.pages = std::move(pages);
    document.blocks = LayoutAnalyzer().analyze(document.pages);

    const Dehyphenator dehyphenator = buildDehyphenator(document.pages);
    std::vector<Paragraph> paragraphs = groupParagraphs(document.blocks, document.pages);

    // The document's language is the one most of its body text is in: an
    // article may open with abstracts in two languages.
    QHash<QString, int> languageChars;
    for (Paragraph &paragraph : paragraphs) {
        assemble(paragraph, document.blocks, document.pages, dehyphenator);
        Normalizer::normalize(paragraph.text);
        if (paragraph.kind == BlockKind::Body)
            languageChars[LanguageDetector::detect(paragraph.text.text())] += paragraph.text.size();
    }
    languageChars.remove(QString());
    int bestChars = 0;
    for (auto it = languageChars.constBegin(); it != languageChars.constEnd(); ++it) {
        if (it.value() > bestChars) {
            bestChars = it.value();
            document.lang = it.key();
        }
    }

    const Segmenter segmenter(options.maxSentenceLength);
    for (const Paragraph &paragraph : paragraphs) {
        const QString &text = paragraph.text.text();
        if (text.isEmpty())
            continue;

        std::vector<TextSpan> spans;
        if (paragraph.kind == BlockKind::Heading && text.size() <= options.maxSentenceLength)
            spans.push_back({0, text.size()});
        else
            spans = segmenter.split(text);

        for (size_t s = 0; s < spans.size(); ++s) {
            Sentence sentence;
            sentence.spoken = text.mid(spans[s].start, spans[s].length);
            for (int source : paragraph.text.sources(spans[s].start, spans[s].length))
                sentence.source.push_back(paragraph.refs[size_t(source)]);
            sentence.page = sentence.source.empty()
                                ? document.blocks[size_t(paragraph.blocks.front())].page
                                : sentence.source.front().page;
            sentence.kind = paragraph.kind;
            sentence.lang = document.lang;
            sentence.paragraphEnd = s + 1 == spans.size();
            document.sentences.push_back(std::move(sentence));
        }
    }
    return document;
}

bool Pipeline::hasTextLayer(const std::vector<PageText> &pages)
{
    size_t words = 0;
    for (const PageText &page : pages)
        words += page.words.size();
    return words > 0 && words >= 5 * pages.size();
}

bool Pipeline::readPdf(const QString &path, ReadingDocument *document, QString *errorMessage,
                       const PipelineOptions &options)
{
    PdfDocument::Error error = PdfDocument::Error::None;
    const std::unique_ptr<PdfDocument> pdf = PdfDocument::open(path, &error);
    if (!pdf) {
        if (errorMessage)
            *errorMessage = PdfDocument::errorString(error);
        return false;
    }

    std::vector<PageText> pages = pdf->allPageText();
    if (!hasTextLayer(pages)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("This PDF has no text layer. Scanned PDFs aren't supported yet.");
        return false;
    }

    *document = run(std::move(pages), options);
    document->title = pdf->title();
    if (document->title.isEmpty())
        document->title = QFileInfo(path).completeBaseName();
    return true;
}

} // namespace lukija
