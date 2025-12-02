import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

Column {
    id: heightmapSettings
    spacing: 10
    
    required property var terrainManager

    Text {
        text: "Fichier heightmap"
        color: "#d8dee9"
        font.pixelSize: 13
        font.weight: Font.Medium
    }

    Rectangle {
        width: parent.width
        height: 40
        radius: 6
        color: imageMouseArea.pressed ? "#3a3d42" : (imageMouseArea.containsMouse ? "#272a2f" : "#212429")
        border.width: 1
        border.color: "#3a3d42"

        Row {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: 16
                height: 16
                source: "../images/layer-plus.svg"
                fillMode: Image.PreserveAspectFit
                opacity: 0.7
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: terrainManager.heightmapSource.toString().length > 0
                    ? terrainManager.heightmapSource.toString().split('/').pop()
                    : "Choisir une image..."
                color: terrainManager.heightmapSource.toString().length > 0 ? "#e5e9f0" : "#a0a0a0"
                font.pixelSize: 12
                elide: Text.ElideRight
                width: parent.width - 50
            }
        }

        MouseArea {
            id: imageMouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: fileDialog.open()
        }
    }

    FileDialog {
        id: fileDialog
        title: "Choisir une image heightmap"
        onAccepted: {
            terrainManager.heightmapSource = fileDialog.selectedFile
        }
    }
}
