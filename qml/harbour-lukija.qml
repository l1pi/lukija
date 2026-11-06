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

    // Playing, or paused for other audio and about to resume.
    readonly property bool audioActive: reader.player.playing || reader.player.interrupted

    // Without this the phone suspends when the screen goes off, and synthesis
    // and playback, or a voice download, stop with it.
    KeepAlive {
        enabled: app.audioActive || voices.busy
    }

    // The audio policy. Other audio takes the resource for a while: calls, and
    // also Android apps, whose sounds (WhatsApp's "message sent", say) count as
    // a player like this app. Playback pauses, keeps its claim, and resumes
    // when the resource comes back soon enough.
    Permissions {
        enabled: app.audioActive
        autoRelease: false  // keep the claim when lost, so "granted" follows
        applicationClass: "player"

        Resource {
            type: Resource.AudioPlayback
        }

        onGranted: reader.player.endInterruption()
        onLost: reader.player.interrupt()
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
