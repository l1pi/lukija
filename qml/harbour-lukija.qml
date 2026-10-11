import QtQuick 2.0
import Sailfish.Silica 1.0
import Amber.Mpris 1.0
import Nemo.KeepAlive 1.2
import Nemo.Policy 1.0
import Lukija 1.0
import "pages"

ApplicationWindow {
    id: app

    function openDocument(path) {
        reader.open(path)
        pageStack.push(Qt.resolvedUrl("pages/ReaderPage.qml"))
    }

    initialPage: Component { LibraryPage { } }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    allowedOrientations: defaultAllowedOrientations

    // Without this the phone suspends when the screen goes off, and synthesis
    // and playback stop with it.
    KeepAlive {
        enabled: reader.player.playing
    }

    // The audio policy: playback pauses for calls and other higher-priority audio.
    Permissions {
        enabled: reader.player.playing
        autoRelease: true
        applicationClass: "player"

        Resource {
            type: Resource.AudioPlayback
        }

        onLost: reader.player.pause()
        onDenied: reader.player.pause()
    }

    // Lock screen and headset controls, showing the sentence being read.
    MprisPlayer {
        readonly property bool ready: reader.status === Reader.Ready

        serviceName: "lukija"
        identity: "Lukija"
        desktopEntry: "harbour-lukija"
        canControl: true
        canPlay: ready
        canPause: ready
        canGoNext: ready
        canGoPrevious: ready
        canSeek: false
        playbackStatus: reader.player.playing ? Mpris.Playing : (ready ? Mpris.Paused : Mpris.Stopped)
        metaData.title: reader.title
        metaData.contributingArtist: reader.sentenceText(reader.player.currentSentence)

        onPlayRequested: reader.player.play()
        onPauseRequested: reader.player.pause()
        onPlayPauseRequested: reader.player.toggle()
        onStopRequested: reader.player.pause()
        onNextRequested: reader.player.next()
        onPreviousRequested: reader.player.previous()
    }

    Component.onCompleted: {
        if (initialFile)
            openDocument(initialFile)
    }
}
