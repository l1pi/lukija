#pragma once

#include "ttsengine.h"

#include <QMutex>
#include <QObject>
#include <QThread>
#include <QVector>

#include <atomic>
#include <deque>
#include <functional>
#include <memory>

namespace lukija {

using EngineFactory = std::function<std::unique_ptr<TtsEngine>()>;

class SynthesisWorker;

// Synthesises sentences on a worker thread that owns the engines: one per
// voice, loaded on first use and kept. Results arrive in request order.
class SynthesisQueue : public QObject
{
    Q_OBJECT

public:
    explicit SynthesisQueue(EngineFactory factory = defaultEngineFactory(), QObject *parent = nullptr);
    ~SynthesisQueue() override;

    // Piper when built with it, otherwise a factory that returns nullptr.
    static EngineFactory defaultEngineFactory();

    void request(int index, const QString &text, const VoiceInfo &voice, float speed);

    // Drops the pending requests and stops the one in progress. Nothing
    // requested before the call is reported after it.
    void cancel();

    int pending() const { return m_pending; }

signals:
    void ready(int index, const QVector<float> &pcm, int sampleRate, qint64 elapsedMs);
    void failed(int index, const QString &error);

private:
    friend class SynthesisWorker;

    struct Request {
        quint64 generation;
        int index;
        QString text;
        VoiceInfo voice;
        float speed;
    };

    // Shared with the worker, guarded by m_mutex.
    QMutex m_mutex;
    std::deque<Request> m_requests;
    quint64 m_generation = 0;
    std::atomic<bool> m_cancel{false};

    QThread m_thread;
    SynthesisWorker *m_worker;
    int m_pending = 0;  // current-generation requests not yet reported

    void onReady(quint64 generation, int index, const QVector<float> &pcm, int sampleRate,
                 qint64 elapsedMs);
    void onFailed(quint64 generation, int index, const QString &error);
};

} // namespace lukija
