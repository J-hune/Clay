import QtQuick
import QtQuick.Controls

// Menu contextuel pour les brushes
Rectangle {
    id: contextMenu

    property int brushIndex: -1
    property bool isVisible: false
    required property var brushManager

    width: 212
    height: menuColumn.implicitHeight + 16
    color: "#272a2f"
    radius: 10
    border.width: 1
    border.color: "#3a3d42"

    visible: isVisible
    opacity: isVisible ? 1.0 : 0.0
    scale: isVisible ? 1.0 : 0.95

    // Animations
    Behavior on opacity { NumberAnimation { duration: 150 } }
    Behavior on scale { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }

    Column {
        id: menuColumn
        anchors.fill: parent
        anchors.margins: 8
        spacing: 4

        // Bouton "Afficher dans le dossier"
        Rectangle {
            width: parent.width
            height: 36
            radius: 6
            color: {
                if (showMouseArea.pressed) return "#3a3d42"
                if (showMouseArea.containsMouse) return "#2e3440"
                return "transparent"
            }

            Behavior on color { ColorAnimation { duration: 100 } }

            Row {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 10

                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    sourceSize.width: 16
                    sourceSize.height: 16
                    source: "../images/folder-magnifying-glass.svg"
                    fillMode: Image.PreserveAspectFit
                    opacity: 0.7
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Afficher dans le dossier"
                    color: "#e5e9f0"
                    font.pixelSize: 13
                }
            }

            MouseArea {
                id: showMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    brushManager.requestShowBrushInFolder(contextMenu.brushIndex)
                    contextMenu.isVisible = false
                }
            }
        }

        // Séparateur
        Rectangle {
            width: parent.width
            height: 1
            color: "#3a3d42"
        }

        // Bouton "Supprimer"
        Rectangle {
            width: parent.width
            height: 36
            radius: 6
            color: {
                if (deleteMouseArea.pressed) return "#bf616a"
                if (deleteMouseArea.containsMouse) return "#b54850"
                return "transparent"
            }

            Behavior on color { ColorAnimation { duration: 100 } }

            Row {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 10

                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    sourceSize.width: 16
                    sourceSize.height: 16
                    source: "../images/circle-xmark.svg"
                    fillMode: Image.PreserveAspectFit
                    opacity: 0.9
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Supprimer"
                    color: deleteMouseArea.containsMouse ? "#ffffff" : "#e5e9f0"
                    font.pixelSize: 13
                    font.weight: deleteMouseArea.containsMouse ? Font.Medium : Font.Normal

                    Behavior on color { ColorAnimation { duration: 100 } }
                }
            }

            MouseArea {
                id: deleteMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    brushManager.requestRemoveBrush(contextMenu.brushIndex)
                    contextMenu.isVisible = false
                }
            }
        }
    }

    // Fonction pour ouvrir le menu à une position donnée
    function openAt(x, y, index) {
        contextMenu.x = x
        contextMenu.y = y

        const winItem = contextMenu.Window.contentItem
        const localPos = mapToItem(winItem, 0, 0)

        // On ajuste si le menu dépasse les bords de la fenêtre
        if (localPos.x + contextMenu.width > winItem.width) {
            contextMenu.x -= (localPos.x + contextMenu.width) - winItem.width + 10
        }
        if (localPos.y + contextMenu.height > winItem.height) {
            contextMenu.y -= (localPos.y + contextMenu.height) - winItem.height + 10
        }

        brushIndex = index
        isVisible = true
    }

    function close() {
        isVisible = false
    }
}
