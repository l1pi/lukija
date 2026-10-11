#include "speaker.h"

#include "audiosink.h"
#include "pcm.h"
#include "playbackcontroller.h"

#include <QDataStream>
#include <QElapsedTimer>
#include <QFile>
#include <QTextStream>

namespace lukija {

namespace {

QTextStream &err()
{
    static QTextStream stream(stderr);
    return stream;
}

void writeWavHeader(QFile &file, quint32 dataBytes)
{
    const int rate = AudioSink::SampleRate;
    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);
    out.writeRawData("RIFF", 4);
    out << quint32(36 + dataBytes);
    out.writeRawData("WAVEfmt ", 8);
    out << quint32(16) << quint16(1) << quint16(1) << quint32(rate) << quint32(rate * 2)
        << quint16(2) << quint16(16);
    out.writeRawData("data", 4);
    out << dataBytes;
}

} // namespace

void printSentence(int index, const Sentence &sentence)
{
    err() << '[' << index << "] " << sentence.spoken << '\n';
    err().flush();
}

void printSummary(qint64 synthesisMs, double audioSeconds)
{
    if (audioSeconds <= 0)
        return;
    err() << QStringLiteral("Synthesised %1 s of audio in %2 s (real-time factor %3)\n")
                 .arg(audioSeconds, 0, 'f', 1)
                 .arg(synthesisMs / 1000.0, 0, 'f', 1)
                 .arg(synthesisMs / 1000.0 / audioSeconds, 0, 'f', 3);
    err().flush();
}

bool speakToWav(const ReadingDocument &document, TtsEngine &engine, const SpeakOptions &options,
                const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        *error = QStringLiteral("Cannot write '%1'").arg(path);
        return false;
    }
    writeWavHeader(file, 0);

    SynthOptions synthOptions;
    synthOptions.speed = options.speed;
    const std::atomic<bool> cancel(false);
    quint32 dataBytes = 0;
    qint64 synthesisMs = 0;
    double audioSeconds = 0;

    for (int i = options.first; i < options.end; ++i) {
        const Sentence &sentence = document.sentences[size_t(i)];
        printSentence(i, sentence);
        QVector<float> samples;
        QElapsedTimer timer;
        timer.start();
        if (!engine.synthesize(sentence.spoken, synthOptions, &samples, cancel)) {
            *error = QStringLiteral("Synthesis failed for sentence %1").arg(i);
            return false;
        }
        synthesisMs += timer.elapsed();
        audioSeconds += double(samples.size()) / engine.sampleRate();

        const QByteArray audio =
            pcm::toInt16(pcm::resample(samples, engine.sampleRate(), AudioSink::SampleRate))
            + pcm::silence(pauseAfterMs(sentence), AudioSink::SampleRate);
        file.write(audio);
        dataBytes += quint32(audio.size());
    }

    file.seek(0);
    writeWavHeader(file, dataBytes);
    printSummary(synthesisMs, audioSeconds);
    return true;
}

} // namespace lukija
