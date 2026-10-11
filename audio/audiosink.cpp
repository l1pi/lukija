#include "audiosink.h"

#include "pcm.h"

#include <QAudioDeviceInfo>
#include <QAudioFormat>
#include <QAudioOutput>

#include <cstring>

namespace lukija {

AudioSink::AudioSink(QObject *parent)
    : QIODevice(parent)
{
    m_positionTimer.setInterval(50);
    connect(&m_positionTimer, &QTimer::timeout, this, &AudioSink::updatePosition);
}

AudioSink::~AudioSink()
{
    stop();
}

bool AudioSink::start(QString *error)
{
    QAudioFormat format;
    format.setSampleRate(SampleRate);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec(QStringLiteral("audio/pcm"));
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    const QAudioDeviceInfo device = QAudioDeviceInfo::defaultOutputDevice();
    if (device.isNull() || !device.isFormatSupported(format)) {
        if (error)
            *error = QStringLiteral("No audio output device supports 22.05 kHz 16-bit mono");
        return false;
    }

    m_output = new QAudioOutput(device, format, this);
    m_output->setBufferSize(SampleRate * 2 / 4); // 250 ms
    open(QIODevice::ReadOnly);
    m_output->start(this);
    if (m_output->error() != QAudio::NoError) {
        if (error)
            *error = QStringLiteral("Could not start audio output (error %1)").arg(m_output->error());
        stop();
        return false;
    }
    m_positionTimer.start();
    return true;
}

void AudioSink::stop()
{
    m_positionTimer.stop();
    if (m_output) {
        m_output->stop();
        delete m_output;
        m_output = nullptr;
    }
    if (isOpen())
        close();
    m_paused = false;
}

bool AudioSink::restart(QString *error)
{
    stop();
    clear();
    m_samplesRead = 0;
    return start(error);
}

void AudioSink::pause()
{
    if (m_output && !m_paused) {
        m_output->suspend();
        m_paused = true;
    }
}

void AudioSink::resume()
{
    if (m_output && m_paused) {
        m_output->resume();
        m_paused = false;
    }
}

void AudioSink::enqueue(int sentence, const QVector<float> &samples, int sampleRate, int pauseAfterMs)
{
    const QByteArray audio = pcm::toInt16(pcm::resample(samples, sampleRate, SampleRate))
                             + pcm::silence(pauseAfterMs, SampleRate);
    const qint64 start = m_samplesRead + queuedBytes() / 2;
    m_segments.push_back({sentence, start, start + audio.size() / 2});
    m_buffer += audio;
    m_finishing = false;
    m_finishedEmitted = false;
}

void AudioSink::finish()
{
    m_finishing = true;
}

void AudioSink::clear()
{
    m_buffer.clear();
    m_bufferOffset = 0;
    m_segments.clear();
    m_current = -1;
    m_finishing = false;
    m_finishedEmitted = false;
}

qint64 AudioSink::queuedMs() const
{
    return queuedBytes() / 2 * 1000 / SampleRate;
}

qint64 AudioSink::readData(char *data, qint64 maxSize)
{
    maxSize &= ~qint64(1); // whole samples only
    qint64 count = qMin(queuedBytes(), maxSize);
    std::memcpy(data, m_buffer.constData() + m_bufferOffset, size_t(count));
    m_bufferOffset += int(count);

    // Drop the consumed front now and then rather than on every read.
    if (m_bufferOffset > (1 << 20) && m_bufferOffset * 2 > m_buffer.size()) {
        m_buffer.remove(0, m_bufferOffset);
        m_bufferOffset = 0;
    }

    // Out of audio: keep the device running on silence rather than letting it
    // stop. Synthesis may just be behind; and after the last sentence the
    // device only reports the tail as played while it keeps receiving data.
    if (count < maxSize) {
        std::memset(data + count, 0, size_t(maxSize - count));
        count = maxSize;
    }

    m_samplesRead += count / 2;
    return count;
}

qint64 AudioSink::writeData(const char *, qint64)
{
    return -1;
}

qint64 AudioSink::bytesAvailable() const
{
    return qMax<qint64>(queuedBytes() + QIODevice::bytesAvailable(), 4096);
}

void AudioSink::updatePosition()
{
    if (!m_output)
        return;

    const qint64 played = m_output->processedUSecs() * SampleRate / 1000000;
    while (!m_segments.empty() && m_segments.front().end <= played)
        m_segments.pop_front();

    if (!m_segments.empty() && m_segments.front().start <= played
        && m_segments.front().sentence != m_current) {
        m_current = m_segments.front().sentence;
        emit currentSentenceChanged(m_current);
    }

    if (m_finishing && m_segments.empty() && queuedBytes() == 0 && !m_finishedEmitted) {
        m_finishedEmitted = true;
        emit finished();
    }
}

} // namespace lukija
