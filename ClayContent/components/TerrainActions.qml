import QtQuick
import QtQuick.Controls

Row {
    id: terrainActions
    spacing: 10
    
    required property var terrainManager

    Rectangle {
        width: (parent.width - 10) / 2
        height: 40
        radius: 6
        color: generateMouseArea.pressed ? "#5e81ac" : (generateMouseArea.containsMouse ? "#4c72a0" : "#4c566a")
        border.width: 0

        Behavior on color {
            ColorAnimation { duration: 100 }
        }

        Text {
            anchors.centerIn: parent
            text: terrainManager.ready ? "Régénérer" : "Générer"
            color: "#eceff4"
            font.pixelSize: 13
            font.weight: Font.DemiBold
        }

        MouseArea {
            id: generateMouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: terrainManager.generate()
        }
    }

    Rectangle {
        width: (parent.width - 10) / 2
        height: 40
        radius: 6
        color: "transparent"
        border.width: 1
        border.color: "#4c566a"

        Column {
            anchors.centerIn: parent
            spacing: 2

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: terrainManager.ready ? "Prêt" : "Non généré"
                color: terrainManager.ready ? "#a3be8c" : "#a0a0a0"
                font.pixelSize: 11
                font.weight: Font.Medium
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: terrainManager.ready ? (terrainManager.triangleCount + " triangles") : ""
                color: "#787878"
                font.pixelSize: 10
                visible: terrainManager.ready
            }
        }
    }
}
