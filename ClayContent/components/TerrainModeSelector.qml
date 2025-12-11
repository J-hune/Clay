import QtQuick
import QtQuick.Controls

Column {
    id: modeSelector
    spacing: 10
    
    required property var terrainManager

    Text {
        text: "Mode de génération"
        color: "#d8dee9"
        font.pixelSize: 13
        font.weight: Font.Medium
    }

    Row {
        spacing: 8
        width: parent.width

        Repeater {
            model: ["Plat", "Heightmap", "Bruit"]

            Rectangle {
                width: (parent.width - 16) / 3
                height: 36
                radius: 6
                color: {
                    var isSelected = (index === terrainManager.mode)
                    var mouseArea = modeMouseAreas.itemAt(index)
                    if (!mouseArea) return isSelected ? "#3a3d42" : "transparent"
                    if (mouseArea.pressed) return "#3a3d42"
                    if (isSelected) return "#2e3440"
                    if (mouseArea.containsMouse) return "#272a2f"
                    return "transparent"
                }
                border.width: 1
                border.color: (index === terrainManager.mode) ? "#4c566a" : "#3a3d42"
                opacity: index === 2 ? 0.5 : 1.0

                Behavior on color {
                    ColorAnimation { duration: 100 }
                }

                Text {
                    anchors.centerIn: parent
                    text: modelData + (index === 2 ? " (bientôt)" : "")
                    color: (index === terrainManager.mode) ? "#e5e9f0" : "#a0a0a0"
                    font.pixelSize: 13
                    font.weight: (index === terrainManager.mode) ? Font.Medium : Font.Normal
                }

                MouseArea {
                    id: modeMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: index === 2 ? Qt.ForbiddenCursor : Qt.PointingHandCursor
                    enabled: index !== 2
                    onClicked: {
                        if (index !== 2) {
                            terrainManager.mode = index
                        }
                    }

                    Component.onCompleted: {
                        modeMouseAreas.model = modeMouseAreas.model || []
                    }
                }

                Component.onCompleted: {
                    if (!modeMouseAreas.itemAt) {
                        modeMouseAreas.model = []
                    }
                }
            }
        }

        Repeater {
            id: modeMouseAreas
            model: 0
        }
    }
}
