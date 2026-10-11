#pragma once

#include "model/document.h"

#include <QByteArray>

namespace lukija {

// Binary form of a ReadingDocument for the extraction cache. Blocks aren't
// stored: playback and highlighting need only the words and sentences.
QByteArray serialize(const ReadingDocument &document);

// Fails on data from another format or pipeline version, so a pipeline
// change invalidates the cache.
bool deserialize(const QByteArray &data, ReadingDocument *document);

} // namespace lukija
