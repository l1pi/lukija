#pragma once

#include "audiosink.h"
#include "model/document.h"
#include "synthesisqueue.h"

#include <QHash>
#include <QObject>
#include <QTimer>

#include <memory>

namespace lukija {

// Silence after a sentence, by its role.
int pauseAfterMs(const Sentence &sentence);

// Reads a document aloud from a sentence onwards: synthesis runs on the
// queue's thread about 20 s ahead of what is audible, and currentSentence
// follows the audio.
class PlaybackController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool playing READ isPlaying NOTIFY playingChanged)
    Q_PROPERTY(bool buffering READ isBuffering NOTIFY bufferingChanged)
    Q_PROPERTY(bool interrupted READ isInterrupted NOTIFY interruptedChanged)
    Q_PROPERTY(int currentSentence READ currentSentence NOTIFY currentSentenceChanged)
    Q_PROPERTY(int sentenceCount READ sentenceCount NOTIFY documentChanged)
    Q_PROPERTY(qreal speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)

public:
    static constexpr qreal MinimumSpeed = 0.5;
    static constexpr qreal MaximumSpeed = 3.0;
    // How long playback waits to resume after an interruption.
    static constexpr int ResumeWithinMs = 30000;

    explicit PlaybackController(QObject *parent = nullptr);
    PlaybackController(EngineFactory engineFactory, QObject *parent);
    ~PlaybackController() override;

    // Stops playback and positions at startSentence.
    void setDocument(std::shared_ptr<const ReadingDocument> document, int startSentence = 0);
    const ReadingDocument *document() const { return m_document.get(); }

    bool isPlaying() const { return m_playing; }
    bool isBuffering() const { return m_buffering; }
    // Paused by interrupt(), and will resume if endInterruption() follows in time.
    bool isInterrupted() const { return m_interrupted; }
    void setResumeWithin(int ms) { m_interruption.setInterval(ms); }
    int currentSentence() const { return m_current; }
    int sentenceCount() const;
    qreal speed() const { return m_speed; }
    void setSpeed(qreal speed);

    // Reads every sentence with this voice, whatever its language. An invalid
    // voice goes back to choosing by language.
    void setVoice(const VoiceInfo &voice);

    QString errorString() const { return m_errorString; }

    // Totals for the real-time factor: synthesis time and audio produced.
    qint64 synthesisMs() const { return m_synthesisMs; }
    double synthesisedSeconds() const { return m_synthesisedSeconds; }

public slots:
    void play();
    void pause();
    void toggle();
    void seek(int sentence);
    void next();
    void previous();
    void nextParagraph();
    void previousParagraph();

    // Other audio takes over for a moment, such as a call or an Android app's
    // notification sound: pauses, and endInterruption() resumes. Pausing or
    // playing in between, or waiting longer than ResumeWithinMs, cancels that.
    void interrupt();
    void endInterruption();

signals:
    void playingChanged();
    void bufferingChanged();
    void interruptedChanged();
    void currentSentenceChanged();
    void documentChanged();
    void speedChanged();
    void errorStringChanged();
    void finished();  // played the last sentence

private:
    void restartAt(int sentence);
    void stopAudio();
    void requestMore();
    void onReady(int index, const QVector<float> &pcm, int sampleRate, qint64 elapsedMs);
    void onFailed(int index, const QString &error);
    void onSinkSentence(int index);
    void onSinkFinished();
    void suspend();
    void setPlaying(bool playing);
    void setInterrupted(bool interrupted);
    void setCurrent(int sentence);
    void setError(const QString &error);
    void updateBuffering();
    VoiceInfo voiceFor(const Sentence &sentence);

    std::shared_ptr<const ReadingDocument> m_document;
    SynthesisQueue m_queue;
    AudioSink m_sink;
    QTimer m_refill;
    QTimer m_interruption;
    QHash<QString, VoiceInfo> m_voices;  // by language
    VoiceInfo m_voice;                   // set by setVoice()

    int m_current = 0;      // sentence playing, or to play next
    int m_nextRequest = 0;  // next sentence to synthesise
    int m_nextEnqueue = 0;  // next sentence the sink expects; results arrive in order
    bool m_playing = false;
    bool m_buffering = false;
    bool m_interrupted = false;
    bool m_streaming = false;  // the sink holds audio from m_current onwards
    bool m_atEnd = false;
    qreal m_speed = 1.0;
    QString m_errorString;
    qint64 m_synthesisMs = 0;
    double m_synthesisedSeconds = 0;
};

} // namespace lukija
