import QtQuick 2.0
import Sailfish.Silica 1.0
import Lukija 1.0

Page {
    id: page

    readonly property QtObject player: reader.player
    readonly property bool ready: reader.status === Reader.Ready

    // Jumps there, instead of scrolling through everything before it.
    function showCurrentSentence() {
        scroll.stop()
        list.positionViewAtIndex(player.currentSentence, ListView.Center)
    }

    // Keeps the sentence being read in view, but leaves the text where it is
    // while that sentence is fully visible, e.g. one the user just tapped.
    // Otherwise it goes about a fifth of the way down: smoothly from nearby,
    // and at once from far away.
    function followCurrentSentence() {
        if (!ready || list.moving || list.count === 0)
            return
        var index = player.currentSentence
        var x = list.width / 2
        var top = list.indexAt(x, list.contentY + 1)
        var bottom = list.indexAt(x, list.contentY + list.height - 1)
        if (bottom < 0)
            bottom = list.count  // past the last sentence
        if (index > top && index < bottom)
            return

        var from = list.contentY
        scroll.stop()
        list.positionViewAtIndex(index, ListView.Beginning)
        var end = list.originY + Math.max(0, list.contentHeight - list.height)
        var to = Math.max(list.originY, Math.min(end, list.contentY - list.height * 0.2))
        if (Math.abs(to - from) > list.height * 2) {
            list.contentY = to
            return
        }
        list.contentY = from
        scroll.to = to
        scroll.start()
    }

    allowedOrientations: Orientation.All

    // The document was already open.
    onStatusChanged: {
        if (status === PageStatus.Activating && ready)
            showCurrentSentence()
    }

    // The document finished loading after this page opened.
    Connections {
        target: reader
        onOpened: page.showCurrentSentence()
    }

    Connections {
        target: player
        onCurrentSentenceChanged: page.followCurrentSentence()
    }

    NumberAnimation {
        id: scroll

        target: list
        property: "contentY"
        duration: 500
        easing.type: Easing.InOutQuad
    }

    SilicaListView {
        id: list

        anchors {
            fill: parent
            bottomMargin: panel.visibleSize
        }
        clip: true
        // Always this model, empty until the document loads.
        model: reader.sentences

        // The user takes over.
        onMovementStarted: scroll.stop()

        // The panel below opens after the list has been positioned, and could
        // cover the last sentences. Turning the phone changes the height too.
        onHeightChanged: {
            if (!moving)
                positionViewAtIndex(player.currentSentence, ListView.Contain)
        }

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

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: voices.installedCount === 0
                text: qsTr("Download voices")
                onClicked: pageStack.push(Qt.resolvedUrl("VoicesPage.qml"))
            }
        }
    }
}
