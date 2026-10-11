import QtQuick 2.0
import Sailfish.Silica 1.0
import Lukija 1.0

CoverBackground {
    readonly property QtObject player: reader.player
    readonly property bool ready: reader.status === Reader.Ready

    CoverPlaceholder {
        visible: !ready
        text: "Lukija"
        icon.source: "/usr/share/icons/hicolor/86x86/apps/harbour-lukija.png"
    }

    Column {
        visible: ready
        x: Theme.paddingLarge
        y: Theme.paddingLarge
        width: parent.width - 2 * x
        spacing: Theme.paddingSmall

        Label {
            width: parent.width
            text: reader.title
            wrapMode: Text.Wrap
            maximumLineCount: 2
            elide: Text.ElideRight
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.highlightColor
        }

        Label {
            width: parent.width
            text: reader.sentenceText(player.currentSentence)
            wrapMode: Text.Wrap
            maximumLineCount: 6
            elide: Text.ElideRight
            font.pixelSize: Theme.fontSizeExtraSmall
        }
    }

    CoverActionList {
        enabled: ready

        CoverAction {
            iconSource: player.playing ? "image://theme/icon-cover-pause" : "image://theme/icon-cover-play"
            onTriggered: player.toggle()
        }

        CoverAction {
            iconSource: "image://theme/icon-cover-next-song"
            onTriggered: player.next()
        }
    }
}
