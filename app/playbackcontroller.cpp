#include "playbackcontroller.h"

#include "appsettings.h"
#include "voicelibrary.h"

namespace lukija {

namespace {

constexpr qint64 LookaheadMs = 20000;

} // namespace

int pauseAfterMs(const Sentence &sentence)
{
    if (sentence.kind == BlockKind::Heading)
        return 900;
    return sentence.paragraphEnd ? 600 : 250;
}

PlaybackController::PlaybackController(QObject *parent)
    : PlaybackController(SynthesisQueue::defaultEngineFactory(), parent)
{
}

PlaybackController::PlaybackController(EngineFactory engineFactory, QObject *parent)
    : QObject(parent)
    , m_queue(std::move(engineFactory))
{
    connect(&m_queue, &SynthesisQueue::ready, this, &PlaybackController::onReady);
    connect(&m_queue, &SynthesisQueue::failed, this, &PlaybackController::onFailed);
    connect(&m_sink, &AudioSink::currentSentenceChanged, this, &PlaybackController::onSinkSentence);
    connect(&m_sink, &AudioSink::finished, this, &PlaybackController::onSinkFinished);

    m_refill.setInterval(500);
    connect(&m_refill, &QTimer::timeout, this, &PlaybackController::requestMore);

    m_interruption.setSingleShot(true);
    m_interruption.setInterval(ResumeWithinMs);
    connect(&m_interruption, &QTimer::timeout, this, [this]() { setInterrupted(false); });
}

PlaybackController::~PlaybackController()
{
    stopAudio();
}

void PlaybackController::setDocument(std::shared_ptr<const ReadingDocument> document,
                                     int startSentence)
{
    stopAudio();
    setInterrupted(false);
    setPlaying(false);
    m_document = std::move(document);
    m_voices.clear();
    m_atEnd = false;
    setError(QString());
    emit documentChanged();
    setCurrent(qBound(0, startSentence, qMax(0, sentenceCount() - 1)));
}

int PlaybackController::sentenceCount() const
{
    return m_document ? int(m_document->sentences.size()) : 0;
}

void PlaybackController::setSpeed(qreal speed)
{
    speed = qBound(MinimumSpeed, speed, MaximumSpeed);
    if (qFuzzyCompare(speed, m_speed))
        return;
    m_speed = speed;
    emit speedChanged();

    // Audio already synthesised has the old speed: start over from the
    // current sentence.
    if (m_playing)
        restartAt(m_current);
    else
        stopAudio();
}

void PlaybackController::setVoice(const VoiceInfo &voice)
{
    m_voice = voice;
    if (m_playing)
        restartAt(m_current);
    else
        stopAudio();
}

void PlaybackController::play()
{
    if (sentenceCount() == 0 || m_playing)
        return;
    if (m_atEnd) {
        m_atEnd = false;
        setCurrent(0);
    }
    setError(QString());
    setPlaying(true);
    setInterrupted(false);  // after playing: never idle in between
    if (m_streaming && m_sink.isPaused())
        m_sink.resume();
    else
        restartAt(m_current);
    m_refill.start();
}

void PlaybackController::pause()
{
    setInterrupted(false);
    suspend();
}

void PlaybackController::toggle()
{
    if (m_playing)
        pause();
    else
        play();
}

void PlaybackController::seek(int sentence)
{
    if (sentenceCount() == 0)
        return;
    sentence = qBound(0, sentence, sentenceCount() - 1);
    m_atEnd = false;
    setCurrent(sentence);
    if (m_playing)
        restartAt(sentence);
    else
        stopAudio();
}

void PlaybackController::next()
{
    seek(m_current + 1);
}

void PlaybackController::previous()
{
    seek(m_current - 1);
}

void PlaybackController::nextParagraph()
{
    if (!m_document)
        return;
    const auto &sentences = m_document->sentences;
    for (int i = m_current; i + 1 < int(sentences.size()); ++i) {
        if (sentences[size_t(i)].paragraphEnd) {
            seek(i + 1);
            return;
        }
    }
}

void PlaybackController::previousParagraph()
{
    if (!m_document)
        return;
    const auto &sentences = m_document->sentences;
    auto paragraphStart = [&sentences](int i) {
        while (i > 0 && !sentences[size_t(i - 1)].paragraphEnd)
            --i;
        return i;
    };
    int start = paragraphStart(m_current);
    if (start == m_current && start > 0)
        start = paragraphStart(start - 1);
    seek(start);
}

void PlaybackController::interrupt()
{
    if (!m_playing)
        return;
    // Interrupted before it stops playing: a binding to "playing ||
    // interrupted", such as the audio resource's, never sees both false.
    setInterrupted(true);
    suspend();
}

void PlaybackController::endInterruption()
{
    if (m_interrupted)
        play();
}

void PlaybackController::restartAt(int sentence)
{
    m_queue.cancel();
    QString error;
    if (!m_sink.restart(&error)) {
        setError(error);
        m_streaming = false;
        m_refill.stop();
        setPlaying(false);
        return;
    }
    m_streaming = true;
    m_nextRequest = sentence;
    m_nextEnqueue = sentence;
    updateBuffering();
    requestMore();
}

void PlaybackController::stopAudio()
{
    m_queue.cancel();
    m_sink.stop();
    m_sink.clear();
    m_streaming = false;
    m_refill.stop();
    updateBuffering();
}

void PlaybackController::requestMore()
{
    if (!m_playing || !m_streaming || !m_document || m_queue.pending() > 0)
        return;
    if (m_nextRequest >= sentenceCount()) {
        m_sink.finish();
        return;
    }
    if (m_sink.queuedMs() > LookaheadMs)
        return;

    const Sentence &sentence = m_document->sentences[size_t(m_nextRequest)];
    const VoiceInfo voice = voiceFor(sentence);
    if (!voice.isValid()) {
        setError(tr("No voices installed"));
        pause();
        return;
    }
    m_queue.request(m_nextRequest, sentence.spoken, voice, float(m_speed));
    ++m_nextRequest;
}

void PlaybackController::onReady(int index, const QVector<float> &pcm, int sampleRate,
                                 qint64 elapsedMs)
{
    if (!m_document || index != m_nextEnqueue)
        return;
    m_synthesisMs += elapsedMs;
    m_synthesisedSeconds += double(pcm.size()) / sampleRate;
    m_sink.enqueue(index, pcm, sampleRate, pauseAfterMs(m_document->sentences[size_t(index)]));
    ++m_nextEnqueue;
    updateBuffering();
    requestMore();
}

void PlaybackController::onFailed(int index, const QString &error)
{
    Q_UNUSED(index);
    setError(error);
    pause();
    stopAudio();
}

void PlaybackController::onSinkSentence(int index)
{
    setCurrent(index);
    updateBuffering();
}

void PlaybackController::onSinkFinished()
{
    stopAudio();
    setPlaying(false);
    m_atEnd = true;
    emit finished();
}

void PlaybackController::suspend()
{
    if (!m_playing)
        return;
    m_sink.pause();
    m_refill.stop();
    setPlaying(false);
    updateBuffering();
}

void PlaybackController::setPlaying(bool playing)
{
    if (m_playing == playing)
        return;
    m_playing = playing;
    emit playingChanged();
}

void PlaybackController::setInterrupted(bool interrupted)
{
    if (interrupted)
        m_interruption.start();
    else
        m_interruption.stop();
    if (m_interrupted == interrupted)
        return;
    m_interrupted = interrupted;
    emit interruptedChanged();
}

void PlaybackController::setCurrent(int sentence)
{
    if (m_current == sentence)
        return;
    m_current = sentence;
    emit currentSentenceChanged();
}

void PlaybackController::setError(const QString &error)
{
    if (m_errorString == error)
        return;
    m_errorString = error;
    emit errorStringChanged();
}

void PlaybackController::updateBuffering()
{
    // Playing, but the current sentence's audio hasn't been synthesised yet.
    const bool buffering = m_playing && m_streaming && m_nextEnqueue <= m_current;
    if (m_buffering == buffering)
        return;
    m_buffering = buffering;
    emit bufferingChanged();
}

VoiceInfo PlaybackController::voiceFor(const Sentence &sentence)
{
    if (m_voice.isValid())
        return m_voice;

    QString language = sentence.lang.isEmpty() ? m_document->lang : sentence.lang;
    if (language.isEmpty())
        language = QStringLiteral("en");

    auto cached = m_voices.constFind(language);
    if (cached != m_voices.constEnd())
        return cached.value();

    const VoiceInfo voice = chooseVoice(language);
    if (voice.isValid())
        m_voices.insert(language, voice);
    return voice;
}

VoiceInfo PlaybackController::chooseVoice(const QString &language)
{
    // A voice for the language, else any voice: an English abstract in a
    // Finnish paper is better read with a Finnish voice than not at all.
    VoiceInfo voice = VoiceLibrary::voiceFor(language, AppSettings().value(voiceSettingKey(language)).toString());
    if (!voice.isValid()) {
        const QList<VoiceInfo> installed = VoiceLibrary::installedVoices();
        if (!installed.isEmpty())
            voice = installed.first();
    }
    return voice;
}

QString PlaybackController::voiceSettingKey(const QString &language)
{
    return QStringLiteral("voices/") + language;
}

void PlaybackController::reloadVoices()
{
    // Start over only if a voice in use changes, e.g. not when another
    // language's voice finishes downloading during playback.
    bool changed = false;
    for (auto it = m_voices.constBegin(); it != m_voices.constEnd(); ++it) {
        if (chooseVoice(it.key()).modelPath != it.value().modelPath)
            changed = true;
    }
    m_voices.clear();
    if (changed && m_playing && !m_voice.isValid())
        restartAt(m_current);
}

} // namespace lukija
