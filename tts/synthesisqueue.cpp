#include "synthesisqueue.h"

#ifdef LUKIJA_HAVE_PIPER
#include "piperengine.h"
#endif

#include <QElapsedTimer>
#include <QMutexLocker>

#include <map>

namespace lukija {

// Lives on the queue's thread and takes requests from the shared queue.
class SynthesisWorker : public QObject
{
    Q_OBJECT

public:
    SynthesisWorker(SynthesisQueue *queue, EngineFactory factory)
        : m_queue(queue)
        , m_factory(std::move(factory))
    {
    }

public slots:
    void process()
    {
        for (;;) {
            SynthesisQueue::Request request;
            {
                QMutexLocker locker(&m_queue->m_mutex);
                if (m_queue->m_requests.empty())
                    return;
                request = std::move(m_queue->m_requests.front());
                m_queue->m_requests.pop_front();
                // Under the mutex, so a cancel() from now on reaches this request.
                m_queue->m_cancel = false;
            }
            run(request);
        }
    }

signals:
    void ready(quint64 generation, int index, const QVector<float> &pcm, int sampleRate,
               qint64 elapsedMs);
    void failed(quint64 generation, int index, const QString &error);

private:
    void run(const SynthesisQueue::Request &request)
    {
        QString error;
        TtsEngine *engine = engineFor(request.voice, &error);
        if (!engine) {
            emit failed(request.generation, request.index, error);
            return;
        }

        SynthOptions options;
        options.speed = request.speed;
        QVector<float> pcm;
        QElapsedTimer timer;
        timer.start();
        if (engine->synthesize(request.text, options, &pcm, m_queue->m_cancel))
            emit ready(request.generation, request.index, pcm, engine->sampleRate(), timer.elapsed());
        else if (!m_queue->m_cancel)
            emit failed(request.generation, request.index,
                        QStringLiteral("Synthesis failed for sentence %1").arg(request.index));
    }

    TtsEngine *engineFor(const VoiceInfo &voice, QString *error)
    {
        auto it = m_engines.find(voice.modelPath);
        if (it != m_engines.end())
            return it->second.get();

        std::unique_ptr<TtsEngine> engine = m_factory ? m_factory() : nullptr;
        if (!engine) {
            *error = QStringLiteral("No speech engine available");
            return nullptr;
        }
        if (!engine->load(voice, error))
            return nullptr;
        return m_engines.emplace(voice.modelPath, std::move(engine)).first->second.get();
    }

    SynthesisQueue *m_queue;
    EngineFactory m_factory;
    std::map<QString, std::unique_ptr<TtsEngine>> m_engines;  // by model path
};

SynthesisQueue::SynthesisQueue(EngineFactory factory, QObject *parent)
    : QObject(parent)
    , m_worker(new SynthesisWorker(this, std::move(factory)))
{
    qRegisterMetaType<QVector<float>>("QVector<float>");

    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &SynthesisWorker::ready, this, &SynthesisQueue::onReady);
    connect(m_worker, &SynthesisWorker::failed, this, &SynthesisQueue::onFailed);
    m_thread.setObjectName(QStringLiteral("synthesis"));
    m_thread.start();
}

SynthesisQueue::~SynthesisQueue()
{
    cancel();
    m_thread.quit();
    m_thread.wait();
}

EngineFactory SynthesisQueue::defaultEngineFactory()
{
#ifdef LUKIJA_HAVE_PIPER
    return []() -> std::unique_ptr<TtsEngine> { return std::unique_ptr<TtsEngine>(new PiperEngine); };
#else
    return []() -> std::unique_ptr<TtsEngine> { return nullptr; };
#endif
}

void SynthesisQueue::request(int index, const QString &text, const VoiceInfo &voice, float speed)
{
    {
        QMutexLocker locker(&m_mutex);
        m_requests.push_back({m_generation, index, text, voice, speed});
    }
    ++m_pending;
    QMetaObject::invokeMethod(m_worker, "process", Qt::QueuedConnection);
}

void SynthesisQueue::cancel()
{
    QMutexLocker locker(&m_mutex);
    ++m_generation;
    m_requests.clear();
    m_cancel = true;
    m_pending = 0;
}

void SynthesisQueue::onReady(quint64 generation, int index, const QVector<float> &pcm,
                             int sampleRate, qint64 elapsedMs)
{
    {
        QMutexLocker locker(&m_mutex);
        if (generation != m_generation)
            return;
    }
    --m_pending;
    emit ready(index, pcm, sampleRate, elapsedMs);
}

void SynthesisQueue::onFailed(quint64 generation, int index, const QString &error)
{
    {
        QMutexLocker locker(&m_mutex);
        if (generation != m_generation)
            return;
    }
    --m_pending;
    emit failed(index, error);
}

} // namespace lukija

#include "synthesisqueue.moc"
