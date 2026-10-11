import QtQuick 2.0
import Sailfish.Silica 1.0
import Lukija 1.0

Page {
    id: page

    readonly property QtObject player: reader.player
    readonly property bool ready: reader.status === Reader.Ready

    allowedOrientations: Orientation.All

    SilicaListView {
        id: list

        anchors {
            fill: parent
            bottomMargin: panel.visibleSize
        }
        clip: true
        model: page.ready ? reader.sentences : null

        // Keep the sentence being read in view. The list follows playback
        // except while the user is scrolling.
        currentIndex: player.currentSentence
        highlightRangeMode: list.moving ? ListView.NoHighlightRange : ListView.ApplyRange
        preferredHighlightBegin: height * 0.2
        preferredHighlightEnd: height * 0.6
        highlightMoveDuration: 500

        header: PageHeader {
            title: reader.title
            description: page.ready
                         //: Reader header: the sentence being read and the total
                         ? qsTr("Sentence %1 of %2").arg(player.currentSentence + 1).arg(player.sentenceCount)
                         : ""
        }

        delegate: ListItem {
            id: item

            readonly property bool current: index === player.currentSentence

            contentHeight: label.height + (model.paragraphEnd ? Theme.paddingLarge * 2 : Theme.paddingSmall)
            onClicked: {
                player.seek(index)
                player.play()
            }

            Label {
                id: label

                x: Theme.horizontalPageMargin
                y: Theme.paddingSmall / 2
                width: parent.width - 2 * x
                text: model.text
                wrapMode: Text.Wrap
                font.pixelSize: model.heading ? Theme.fontSizeLarge : Theme.fontSizeMedium
                color: item.current || item.highlighted ? Theme.highlightColor : Theme.primaryColor
            }
        }

        VerticalScrollDecorator { }
    }

    BusyLabel {
        text: qsTr("Reading the PDF")
        running: reader.status === Reader.Loading
    }

    ViewPlaceholder {
        enabled: reader.status === Reader.Error
        text: qsTr("Can't read this PDF")
        hintText: reader.errorString
    }

    DockedPanel {
        id: panel

        width: parent.width
        height: controls.height + Theme.paddingLarge
        dock: Dock.Bottom
        open: page.ready

        Column {
            id: controls

            y: Theme.paddingMedium
            width: parent.width

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: Theme.paddingLarge

                IconButton {
                    icon.source: "image://theme/icon-m-previous"
                    onClicked: player.previous()
                    onPressAndHold: player.previousParagraph()
                }

                IconButton {
                    icon.source: player.playing ? "image://theme/icon-l-pause" : "image://theme/icon-l-play"
                    onClicked: player.toggle()

                    BusyIndicator {
                        anchors.centerIn: parent
                        size: BusyIndicatorSize.Large
                        running: player.buffering
                    }
                }

                IconButton {
                    icon.source: "image://theme/icon-m-next"
                    onClicked: player.next()
                    onPressAndHold: player.nextParagraph()
                }
            }

            Slider {
                id: speedSlider

                width: parent.width
                minimumValue: 0.5
                maximumValue: 2.0
                stepSize: 0.1
                value: player.speed
                label: qsTr("Speed")
                valueText: value.toFixed(1) + " ×"
                onDownChanged: {
                    if (!down)
                        player.speed = value
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: text !== ""
                text: player.errorString
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.errorColor
            }
        }
    }
}
