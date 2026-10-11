#pragma once

#include "audiosink.h"
#include "model/document.h"
#include "ttsengine.h"

#include <QElapsedTimer>
#include <QObject>
#include <QThread>
#include <QTimer>

#include <memory>

namespace lukija {

struct SpeakOptions {
    float speed = 1.0f;
    int first = 0;  // sentence index
    int end = 0;    // one past the last sentence
};

// Silence after a sentence, by its role.
int pauseAfterMs(const Sentence &sentence);

// Synthesises the sentences into a 22.05 kHz 16-bit mono WAV file.
bool speakToWav(const ReadingDocument &document, TtsEngine &engine, const SpeakOptions &options,
                const QString &path, QString *error);

// Owns the engine on a worker thread; synthesises one sentence per request.
class SynthWorker : public QObject
{
    Q_OBJECT

public:
    explicit SynthWorker(std::unique_ptr<TtsEngine> engine);
    ~SynthWorker() override;

public slots:
    void synthesize(int index, const QString &text, float speed);

signals:
    void synthesized(int index, const QVector<float> &pcm, int sampleRate, qint64 elapsedMs);
    void failed(int index);

private:
    std::unique_ptr<TtsEngine> m_engine;
    std::atomic<bool> m_cancel{false};
};

// Reads sentences aloud: synthesis runs ahead of playback by up to ~15 s.
class Player : public QObject
{
    Q_OBJECT

public:
    Player(const ReadingDocument &document, std::unique_ptr<TtsEngine> engine,
           const SpeakOptions &options, QObject *parent = nullptr);
    ~Player() override;

    bool start(QString *error);

signals:
    void finished(bool ok);

private:
    void requestMore();
    void onSynthesized(int index, const QVector<float> &pcm, int sampleRate, qint64 elapsedMs);
    void onCurrentSentenceChanged(int index);

    const ReadingDocument &m_document;
    SpeakOptions m_options;
    AudioSink m_sink;
    QThread m_thread;
    SynthWorker *m_worker;
    QTimer m_refill;
    int m_next;
    bool m_inFlight = false;
    qint64 m_synthesisMs = 0;
    double m_audioSeconds = 0;
};

} // namespace lukija
