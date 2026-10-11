#include "speaker.h"

#include "pcm.h"

#include <QDataStream>
#include <QFile>
#include <QTextStream>

namespace lukija {

namespace {

QTextStream &err()
{
    static QTextStream stream(stderr);
    return stream;
}

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

int pauseAfterMs(const Sentence &sentence)
{
    if (sentence.kind == BlockKind::Heading)
        return 900;
    return sentence.paragraphEnd ? 600 : 250;
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

SynthWorker::SynthWorker(std::unique_ptr<TtsEngine> engine)
    : m_engine(std::move(engine))
{
}

SynthWorker::~SynthWorker() = default;

void SynthWorker::synthesize(int index, const QString &text, float speed)
{
    SynthOptions options;
    options.speed = speed;
    QVector<float> pcm;
    QElapsedTimer timer;
    timer.start();
    if (m_engine->synthesize(text, options, &pcm, m_cancel))
        emit synthesized(index, pcm, m_engine->sampleRate(), timer.elapsed());
    else
        emit failed(index);
}

Player::Player(const ReadingDocument &document, std::unique_ptr<TtsEngine> engine,
               const SpeakOptions &options, QObject *parent)
    : QObject(parent)
    , m_document(document)
    , m_options(options)
    , m_worker(new SynthWorker(std::move(engine)))
    , m_next(options.first)
{
    qRegisterMetaType<QVector<float>>("QVector<float>");

    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &SynthWorker::synthesized, this, &Player::onSynthesized);
    connect(m_worker, &SynthWorker::failed, this, [this](int index) {
        err() << "Synthesis failed for sentence " << index << '\n';
        emit finished(false);
    });
    connect(&m_sink, &AudioSink::currentSentenceChanged, this, &Player::onCurrentSentenceChanged);
    connect(&m_sink, &AudioSink::finished, this, [this]() {
        printSummary(m_synthesisMs, m_audioSeconds);
        emit finished(true);
    });

    m_refill.setInterval(500);
    connect(&m_refill, &QTimer::timeout, this, &Player::requestMore);
}

Player::~Player()
{
    m_sink.stop();
    m_thread.quit();
    m_thread.wait();
}

bool Player::start(QString *error)
{
    if (!m_sink.start(error))
        return false;
    m_thread.start();
    m_refill.start();
    requestMore();
    return true;
}

void Player::requestMore()
{
    if (m_inFlight)
        return;
    if (m_next >= m_options.end) {
        m_sink.finish();
        return;
    }
    if (m_sink.queuedMs() > 15000)
        return;

    m_inFlight = true;
    QMetaObject::invokeMethod(m_worker, "synthesize", Qt::QueuedConnection, Q_ARG(int, m_next),
                              Q_ARG(QString, m_document.sentences[size_t(m_next)].spoken),
                              Q_ARG(float, m_options.speed));
    ++m_next;
}

void Player::onSynthesized(int index, const QVector<float> &pcm, int sampleRate, qint64 elapsedMs)
{
    m_inFlight = false;
    m_synthesisMs += elapsedMs;
    m_audioSeconds += double(pcm.size()) / sampleRate;
    m_sink.enqueue(index, pcm, sampleRate, pauseAfterMs(m_document.sentences[size_t(index)]));
    requestMore();
}

void Player::onCurrentSentenceChanged(int index)
{
    printSentence(index, m_document.sentences[size_t(index)]);
}

} // namespace lukija
