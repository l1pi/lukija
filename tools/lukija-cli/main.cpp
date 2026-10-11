// lukija-cli: shows how Lukija will read a PDF, and reads it aloud on desktop.

#include "appinfo.h"
#include "pipeline.h"
#include "playbackcontroller.h"
#include "speaker.h"
#include "voicelibrary.h"

#ifdef LUKIJA_HAVE_PIPER
#include "piperengine.h"
#endif

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTextStream>

using namespace lukija;

namespace {

QTextStream &out()
{
    static QTextStream stream(stdout);
    return stream;
}

QTextStream &err()
{
    static QTextStream stream(stderr);
    return stream;
}

int fail(const QString &message)
{
    err() << "lukija-cli: " << message << '\n';
    return 1;
}

QString blockText(const Block &block, const PageText &page)
{
    QString text;
    for (const Line &line : block.lines) {
        for (int w : line.words) {
            if (!text.isEmpty())
                text += QLatin1Char(' ');
            text += page.words[size_t(w)].text;
        }
    }
    return text;
}

void printHeader(const ReadingDocument &document)
{
    out() << "% title: " << document.title << '\n'
          << "% lang: " << (document.lang.isEmpty() ? QStringLiteral("?") : document.lang) << '\n'
          << '\n';
}

// One sentence per line, a blank line after each paragraph, headings marked
// with "#". This is the format of the corpus golden files.
int dump(const ReadingDocument &document)
{
    printHeader(document);
    for (const Sentence &sentence : document.sentences) {
        if (sentence.kind == BlockKind::Heading)
            out() << "# ";
        out() << sentence.spoken << '\n';
        if (sentence.paragraphEnd)
            out() << '\n';
    }
    return 0;
}

int annotate(const ReadingDocument &document)
{
    printHeader(document);
    int page = -1;
    for (const Block &block : document.blocks) {
        if (block.page != page) {
            page = block.page;
            const QSizeF size = document.pages[size_t(page)].size;
            out() << QStringLiteral("== Page %1 (%2 x %3 pt) ==\n")
                         .arg(page + 1).arg(size.width(), 0, 'f', 0).arg(size.height(), 0, 'f', 0);
        }
        QString text = blockText(block, document.pages[size_t(page)]);
        if (text.size() > 90)
            text = text.left(89) + QChar(0x2026);
        out() << QStringLiteral("%1 x=%2 y=%3 w=%4 h=%5 fs=%6 lines=%7  %8\n")
                     .arg(QString::fromLatin1(blockKindName(block.kind)), -8)
                     .arg(block.box.x(), 5, 'f', 0).arg(block.box.y(), 5, 'f', 0)
                     .arg(block.box.width(), 5, 'f', 0).arg(block.box.height(), 5, 'f', 0)
                     .arg(block.fontSize, 4, 'f', 1).arg(block.lines.size(), 3)
                     .arg(text);
    }
    return 0;
}

int listVoices()
{
    out() << "Voices directory: " << VoiceLibrary::voicesDirectory() << '\n';
    const QList<VoiceInfo> voices = VoiceLibrary::installedVoices();
    if (voices.isEmpty())
        out() << "No voices installed. Get some with tools/fetch-voice.sh\n";
    for (const VoiceInfo &voice : voices)
        out() << QStringLiteral("  %1 (%2)\n").arg(voice.id, -32).arg(voice.language);
    return 0;
}

int speak(const ReadingDocument &document, const QCommandLineParser &parser)
{
#ifdef LUKIJA_HAVE_PIPER
    const QString voiceName = parser.isSet(QStringLiteral("voice"))
                                  ? parser.value(QStringLiteral("voice"))
                                  : VoiceLibrary::defaultVoiceId(document.lang);
    const VoiceInfo voice = VoiceLibrary::find(voiceName);
    if (!voice.isValid())
        return fail(QStringLiteral("voice '%1' not found in %2\nDownload it with: tools/fetch-voice.sh %1")
                        .arg(voiceName, VoiceLibrary::voicesDirectory()));

    SpeakOptions options;
    options.end = int(document.sentences.size());
    bool ok = true;
    if (parser.isSet(QStringLiteral("speed")))
        options.speed = parser.value(QStringLiteral("speed")).toFloat(&ok);
    if (!ok || options.speed < PlaybackController::MinimumSpeed
        || options.speed > PlaybackController::MaximumSpeed)
        return fail(QStringLiteral("--speed must be between %1 and %2")
                        .arg(PlaybackController::MinimumSpeed)
                        .arg(PlaybackController::MaximumSpeed));
    if (parser.isSet(QStringLiteral("from")))
        options.first = parser.value(QStringLiteral("from")).toInt();
    if (parser.isSet(QStringLiteral("page"))) {
        const int page = parser.value(QStringLiteral("page")).toInt() - 1;
        options.first = options.end;
        for (size_t i = 0; i < document.sentences.size(); ++i) {
            if (document.sentences[i].page >= page) {
                options.first = int(i);
                break;
            }
        }
    }
    if (parser.isSet(QStringLiteral("count")))
        options.end = qMin(options.end, options.first + parser.value(QStringLiteral("count")).toInt());
    if (options.first < 0 || options.first >= options.end)
        return fail(QStringLiteral("nothing to read in that range"));

    err() << "Voice: " << voice.id << ", sentences " << options.first << "-" << options.end - 1
          << " of " << document.sentences.size() << '\n';
    err().flush();

    if (parser.isSet(QStringLiteral("output"))) {
        PiperEngine engine;
        QString error;
        if (!engine.load(voice, &error)
            || !speakToWav(document, engine, options, parser.value(QStringLiteral("output")), &error))
            return fail(error);
        return 0;
    }

    // The same playback as in the app, stopping after the requested range.
    PlaybackController player;
    player.setVoice(voice);
    player.setSpeed(options.speed);
    player.setDocument(std::make_shared<const ReadingDocument>(document), options.first);
    QObject::connect(&player, &PlaybackController::currentSentenceChanged, qApp, [&]() {
        const int current = player.currentSentence();
        if (current >= options.end)
            qApp->exit(0);
        else
            printSentence(current, document.sentences[size_t(current)]);
    });
    QObject::connect(&player, &PlaybackController::finished, qApp, []() { qApp->exit(0); });
    QObject::connect(&player, &PlaybackController::errorStringChanged, qApp, [&player]() {
        if (!player.errorString().isEmpty()) {
            fail(player.errorString());
            qApp->exit(1);
        }
    });
    printSentence(options.first, document.sentences[size_t(options.first)]);
    player.play();
    const int status = qApp->exec();
    printSummary(player.synthesisMs(), player.synthesisedSeconds());
    return status;
#else
    Q_UNUSED(document)
    Q_UNUSED(parser)
    return fail(QStringLiteral("built without Piper (LUKIJA_WITH_PIPER=OFF)"));
#endif
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QString::fromLatin1(OrganizationName));
    QCoreApplication::setApplicationName(QString::fromLatin1(ApplicationName));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral(
        "Shows how Lukija reads a PDF, or reads it aloud.\n\n"
        "Commands:\n"
        "  dump <file.pdf>      the text as it will be spoken, one sentence per line\n"
        "  annotate <file.pdf>  layout blocks and how they were classified\n"
        "  speak <file.pdf>     read aloud with a Piper voice\n"
        "  voices               list installed voices"));
    parser.addHelpOption();
    parser.addPositionalArgument(QStringLiteral("command"), QStringLiteral("dump, annotate, speak or voices"));
    parser.addPositionalArgument(QStringLiteral("file"), QStringLiteral("PDF file"));
    parser.addOptions({
        {QStringLiteral("voice"), QStringLiteral("speak: voice id or path to a .onnx model (default: by document language)"), QStringLiteral("voice")},
        {QStringLiteral("speed"), QStringLiteral("speak: speaking speed, e.g. 1.25"), QStringLiteral("factor")},
        {QStringLiteral("from"), QStringLiteral("speak: start at sentence N"), QStringLiteral("N")},
        {QStringLiteral("page"), QStringLiteral("speak: start at page P (from 1)"), QStringLiteral("P")},
        {QStringLiteral("count"), QStringLiteral("speak: read at most N sentences"), QStringLiteral("N")},
        {{QStringLiteral("o"), QStringLiteral("output")}, QStringLiteral("speak: write a WAV file instead of playing"), QStringLiteral("file.wav")},
    });
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    const QString command = args.value(0);
    if (command == QLatin1String("voices"))
        return listVoices();

    if (args.size() != 2 || (command != QLatin1String("dump") && command != QLatin1String("annotate")
                             && command != QLatin1String("speak")))
        parser.showHelp(1);

    ReadingDocument document;
    QString error;
    if (!Pipeline::readPdf(args.at(1), &document, &error))
        return fail(QStringLiteral("%1: %2").arg(args.at(1), error));

    if (command == QLatin1String("dump"))
        return dump(document);
    if (command == QLatin1String("annotate"))
        return annotate(document);
    return speak(document, parser);
}
