import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0

Page {
    id: page

    // Set by the file picker; opened once this page is back on top.
    property string pickedFile

    allowedOrientations: Orientation.All

    onStatusChanged: {
        if (status === PageStatus.Active && pickedFile) {
            var path = pickedFile
            pickedFile = ""
            app.openDocument(path)
        }
    }

    SilicaListView {
        id: list

        anchors.fill: parent
        model: library

        header: PageHeader {
            title: "Lukija"
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("Open PDF")
                onClicked: pageStack.push(filePicker)
            }
        }

        ViewPlaceholder {
            enabled: list.count === 0
            text: qsTr("No documents yet")
            hintText: qsTr("Pull down to open a PDF")
        }

        delegate: ListItem {
            id: item

            contentHeight: Theme.itemSizeMedium
            enabled: model.available
            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Remove from list")
                    onClicked: {
                        var documentId = model.documentId
                        item.remorseDelete(function() { library.remove(documentId) })
                    }
                }
            }
            onClicked: app.openDocument(model.path)

            Column {
                anchors.verticalCenter: parent.verticalCenter
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x

                Label {
                    width: parent.width
                    text: model.title
                    truncationMode: TruncationMode.Fade
                    color: item.highlighted ? Theme.highlightColor : Theme.primaryColor
                    opacity: model.available ? 1.0 : Theme.opacityLow
                }

                Label {
                    width: parent.width
                    text: model.available
                          //: Library list: reading progress and the number of pages
                          ? qsTr("%1 % read, %n pages", "", model.pageCount).arg(Math.round(model.progress * 100))
                          : qsTr("File not found")
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                }
            }
        }

        VerticalScrollDecorator { }
    }

    Component {
        id: filePicker

        FilePickerPage {
            title: qsTr("Open PDF")
            nameFilters: ["*.pdf", "*.PDF"]
            onSelectedContentPropertiesChanged: page.pickedFile = selectedContentProperties.filePath
        }
    }
}
