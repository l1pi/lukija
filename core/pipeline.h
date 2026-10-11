#pragma once

#include "model/document.h"

#include <QString>

namespace lukija {

struct PipelineOptions {
    int maxSentenceLength = 300;
};

// PDF text -> blocks -> paragraphs -> normalised sentences, each sentence
// keeping references to the words it came from.
class Pipeline
{
public:
    static ReadingDocument run(std::vector<PageText> pages, const PipelineOptions &options = {});

    // False for scanned PDFs: a handful of words per page at most, typically
    // page numbers or a publisher's stamp over page images.
    static bool hasTextLayer(const std::vector<PageText> &pages);

    // Opens a PDF and runs the pipeline. On failure returns false and sets
    // errorMessage.
    static bool readPdf(const QString &path, ReadingDocument *document, QString *errorMessage,
                        const PipelineOptions &options = {});
};

} // namespace lukija
