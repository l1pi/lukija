#pragma once

#include "model/document.h"
#include "ttsengine.h"

namespace lukija {

struct SpeakOptions {
    float speed = 1.0f;
    int first = 0;  // sentence index
    int end = 0;    // one past the last sentence
};

// Synthesises the sentences into a 22.05 kHz 16-bit mono WAV file.
bool speakToWav(const ReadingDocument &document, TtsEngine &engine, const SpeakOptions &options,
                const QString &path, QString *error);

void printSentence(int index, const Sentence &sentence);
void printSummary(qint64 synthesisMs, double audioSeconds);

} // namespace lukija
