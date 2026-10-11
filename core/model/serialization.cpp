#include "serialization.h"

#include <QDataStream>

namespace lukija {

namespace {

constexpr quint32 Magic = 0x4c4b4a44; // "LKJD"
constexpr quint32 FormatVersion = 1;

enum WordFlag : quint8 {
    SpaceAfter = 1,
    EndsLine = 2,
};

} // namespace

QByteArray serialize(const ReadingDocument &document)
{
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_6);

    out << Magic << FormatVersion << qint32(document.pipelineVersion);
    out << document.title << document.lang;

    out << quint32(document.pages.size());
    for (const PageText &page : document.pages) {
        out << page.size << quint32(page.words.size());
        for (const Word &word : page.words) {
            const quint8 flags = (word.spaceAfter ? SpaceAfter : 0) | (word.endsLine ? EndsLine : 0);
            out << word.text << word.box << flags;
        }
    }

    out << quint32(document.sentences.size());
    for (const Sentence &sentence : document.sentences) {
        out << sentence.spoken << qint32(sentence.page) << qint32(sentence.kind) << sentence.lang
            << sentence.paragraphEnd << quint32(sentence.source.size());
        for (const WordRef &ref : sentence.source)
            out << qint32(ref.page) << qint32(ref.word);
    }
    return data;
}

bool deserialize(const QByteArray &data, ReadingDocument *document)
{
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_5_6);

    quint32 magic = 0;
    quint32 format = 0;
    qint32 pipelineVersion = 0;
    in >> magic >> format >> pipelineVersion;
    if (magic != Magic || format != FormatVersion || pipelineVersion != PipelineVersion)
        return false;

    ReadingDocument result;
    result.pipelineVersion = pipelineVersion;
    in >> result.title >> result.lang;

    quint32 pageCount = 0;
    in >> pageCount;
    for (quint32 p = 0; p < pageCount && in.status() == QDataStream::Ok; ++p) {
        PageText page;
        quint32 wordCount = 0;
        in >> page.size >> wordCount;
        for (quint32 w = 0; w < wordCount && in.status() == QDataStream::Ok; ++w) {
            Word word;
            quint8 flags = 0;
            in >> word.text >> word.box >> flags;
            word.spaceAfter = flags & SpaceAfter;
            word.endsLine = flags & EndsLine;
            page.words.push_back(std::move(word));
        }
        result.pages.push_back(std::move(page));
    }

    quint32 sentenceCount = 0;
    in >> sentenceCount;
    for (quint32 s = 0; s < sentenceCount && in.status() == QDataStream::Ok; ++s) {
        Sentence sentence;
        qint32 page = 0;
        qint32 kind = 0;
        quint32 sourceCount = 0;
        in >> sentence.spoken >> page >> kind >> sentence.lang >> sentence.paragraphEnd >> sourceCount;
        sentence.page = page;
        sentence.kind = BlockKind(kind);
        for (quint32 r = 0; r < sourceCount && in.status() == QDataStream::Ok; ++r) {
            qint32 refPage = 0;
            qint32 refWord = 0;
            in >> refPage >> refWord;
            sentence.source.push_back({refPage, refWord});
        }
        result.sentences.push_back(std::move(sentence));
    }

    if (in.status() != QDataStream::Ok)
        return false;
    *document = std::move(result);
    return true;
}

} // namespace lukija
