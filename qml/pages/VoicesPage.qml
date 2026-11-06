import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    allowedOrientations: Orientation.All

    SilicaListView {
        id: list

        anchors.fill: parent
        model: voices

        header: Column {
            width: list.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Voices")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.highlightColor
                text: qsTr("Lukija reads with Piper voices, which run on the phone, without a network "
                           + "connection. Download a voice for each language you read.")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: voices.errorString !== ""
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.errorColor
                text: voices.errorString
            }

            Item {
                width: 1
                height: Theme.paddingSmall
            }
        }

        section.property: "languageName"
        section.delegate: SectionHeader {
            text: section
        }

        delegate: ListItem {
            id: item

            readonly property string voiceId: model.voiceId

            contentHeight: content.height + 2 * Theme.paddingMedium
            // One download at a time.
            enabled: model.installed || model.downloading || !voices.busy

            onClicked: {
                if (model.installed)
                    voices.choose(voiceId)
                else if (!model.downloading)
                    voices.download(voiceId)
            }

            menu: ContextMenu {
                MenuItem {
                    visible: model.inCatalog
                    text: qsTr("Listen to a sample")
                    onClicked: Qt.openUrlExternally(model.sampleUrl)
                }
                MenuItem {
                    visible: model.downloading
                    text: qsTr("Cancel download")
                    onClicked: voices.cancel()
                }
                MenuItem {
                    visible: model.installed
                    text: qsTr("Remove")
                    onClicked: {
                        var voiceId = item.voiceId
                        item.remorseDelete(function() { voices.remove(voiceId) })
                    }
                }
            }

            Column {
                id: content

                x: Theme.horizontalPageMargin
                y: Theme.paddingMedium
                width: parent.width - 2 * x - inUseIcon.width - Theme.paddingMedium

                Label {
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    color: item.highlighted || model.inUse ? Theme.highlightColor : Theme.primaryColor
                    //: Voices page: a voice's name and the country of its accent
                    text: model.country !== "" ? qsTr("%1, %2").arg(model.name).arg(model.country)
                                               : model.name
                }

                Label {
                    width: parent.width
                    wrapMode: Text.Wrap
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    text: {
                        if (model.downloading)
                            return qsTr("Downloading, %1 %").arg(Math.floor(model.progress * 100))
                        if (model.inUse)
                            //: Voices page: this voice reads the language, e.g. "Reads English"
                            return qsTr("Reads %1").arg(model.languageName)
                        if (model.installed)
                            return qsTr("Installed. Tap to read %1 with this voice").arg(model.languageName)
                        //: Voices page: %1 is the download size in megabytes
                        return qsTr("Tap to download, %1 MB").arg(Math.round(model.size / 1000000))
                    }
                }

                Item {
                    width: parent.width
                    height: Theme.paddingSmall * 2
                    visible: model.downloading

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width
                        height: Math.max(2, Theme.paddingSmall / 2)
                        radius: height / 2
                        color: Theme.rgba(Theme.highlightColor, Theme.opacityLow)
                    }

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width * model.progress
                        height: Math.max(2, Theme.paddingSmall / 2)
                        radius: height / 2
                        color: Theme.highlightColor
                    }
                }

                Label {
                    width: parent.width
                    visible: model.licence !== ""
                    font.pixelSize: Theme.fontSizeTiny
                    color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    //: Voices page: the licence of the recordings the voice was trained on
                    text: qsTr("Recordings: %1").arg(model.licence)
                }
            }

            Image {
                id: inUseIcon

                anchors {
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                source: "image://theme/icon-m-acknowledge"
                opacity: model.inUse ? 1.0 : 0.0
            }
        }

        footer: Item {
            width: list.width
            height: footerLabel.height + 2 * Theme.paddingLarge

            Label {
                id: footerLabel

                x: Theme.horizontalPageMargin
                y: Theme.paddingLarge
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                text: qsTr("The voices come from the Piper project (huggingface.co/rhasspy/piper-voices). "
                           + "Long press a voice to listen to a sample first.")
            }
        }

        VerticalScrollDecorator { }
    }
}
