#pragma once

#include "ttsengine.h"

#include <QList>

namespace lukija {

// Piper voices installed as <id>.onnx + <id>.onnx.json in the voices directory.
class VoiceLibrary
{
public:
    // <app data>/voices, e.g. ~/.local/share/io.github.l1pi/harbour-lukija/voices
    static QString voicesDirectory();

    static QList<VoiceInfo> installedVoices(const QString &directory = voicesDirectory());

    // Path to a .onnx model, or the id of an installed voice.
    static VoiceInfo find(const QString &idOrPath);

    static VoiceInfo fromModelPath(const QString &modelPath);

    // The voice to use for a language ("fi", "en") when the user hasn't chosen one.
    static QString defaultVoiceId(const QString &language);
};

} // namespace lukija
