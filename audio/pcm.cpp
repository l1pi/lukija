#include "pcm.h"

#include <QtEndian>

#include <cmath>

namespace lukija {
namespace pcm {

QVector<float> resample(const QVector<float> &samples, int fromRate, int toRate)
{
    if (fromRate == toRate || samples.isEmpty())
        return samples;

    const int count = int(std::lround(double(samples.size()) * toRate / fromRate));
    QVector<float> out(count);
    const double step = double(fromRate) / toRate;
    for (int i = 0; i < count; ++i) {
        const double position = i * step;
        const int index = int(position);
        const float fraction = float(position - index);
        const float a = samples.at(qMin(index, samples.size() - 1));
        const float b = samples.at(qMin(index + 1, samples.size() - 1));
        out[i] = a + (b - a) * fraction;
    }
    return out;
}

QByteArray toInt16(const QVector<float> &samples)
{
    QByteArray out(samples.size() * 2, Qt::Uninitialized);
    uchar *data = reinterpret_cast<uchar *>(out.data());
    for (int i = 0; i < samples.size(); ++i) {
        const float clipped = qBound(-1.0f, samples.at(i), 1.0f);
        qToLittleEndian<qint16>(qint16(std::lround(clipped * 32767.0f)), data + 2 * i);
    }
    return out;
}

QByteArray silence(int milliseconds, int sampleRate)
{
    const int samples = int(qint64(milliseconds) * sampleRate / 1000);
    return QByteArray(samples * 2, '\0');
}

} // namespace pcm
} // namespace lukija
