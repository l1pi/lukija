#pragma once

#include <QByteArray>
#include <QVector>

namespace lukija {
namespace pcm {

// Linear interpolation. Good enough for speech going up from 16 kHz voices to
// the 22.05 kHz output rate.
QVector<float> resample(const QVector<float> &samples, int fromRate, int toRate);

// Signed 16-bit little-endian samples, clipped to [-1, 1].
QByteArray toInt16(const QVector<float> &samples);

QByteArray silence(int milliseconds, int sampleRate);

} // namespace pcm
} // namespace lukija
