#pragma once

#include <QByteArray>
#include <QIODevice>
#include <QTimer>
#include <QVector>

#include <deque>

class QAudioOutput;

namespace lukija {

// Plays a queue of synthesised sentences through QAudioOutput (pull mode) and
// reports which sentence is currently audible.
//
// Everything is converted to one output rate, so voices with different sample
// rates can follow each other without reopening the audio device.
class AudioSink : public QIODevice
{
    Q_OBJECT

public:
    static constexpr int SampleRate = 22050;

    explicit AudioSink(QObject *parent = nullptr);
    ~AudioSink() override;

    bool start(QString *error = nullptr);
    void stop();
    // Drops all queued audio, including what the device has buffered, and
    // starts a new stream: for seeking.
    bool restart(QString *error = nullptr);

    void pause();
    void resume();
    bool isPaused() const { return m_paused; }

    void enqueue(int sentence, const QVector<float> &samples, int sampleRate, int pauseAfterMs);
    void finish();  // nothing more will be enqueued; finished() follows once it has played
    void clear();   // drop everything not yet handed to the audio device

    qint64 queuedMs() const;  // audio waiting to be handed to the device
    int currentSentence() const { return m_current; }

signals:
    void currentSentenceChanged(int sentence);
    void finished();

protected:
    qint64 readData(char *data, qint64 maxSize) override;
    qint64 writeData(const char *data, qint64 maxSize) override;
    qint64 bytesAvailable() const override;
    bool isSequential() const override { return true; }

private:
    struct Segment {
        int sentence;
        qint64 start;  // stream position in samples, including the pause after it
        qint64 end;
    };

    qint64 queuedBytes() const { return m_buffer.size() - m_bufferOffset; }
    void updatePosition();

    QAudioOutput *m_output = nullptr;
    QTimer m_positionTimer;
    QByteArray m_buffer;          // int16 samples not yet read by the device
    int m_bufferOffset = 0;       // bytes of m_buffer already read
    qint64 m_samplesRead = 0;     // stream position: samples handed to the device, silence included
    std::deque<Segment> m_segments;
    int m_current = -1;
    bool m_finishing = false;
    bool m_finishedEmitted = false;
    bool m_paused = false;
};

} // namespace lukija
