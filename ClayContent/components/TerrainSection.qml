import QtQuick
import QtQuick.Controls

Rectangle {
    width: parent.width
    height: terrainSection.height
    color: "transparent"

    required property var terrainManager

    Column {
        id: terrainSection
        width: parent.width
        spacing: 8

        // Header section
        Text {
            text: "Terrain"
            color: "#bf5934"
            font.pixelSize: 16
            font.weight: Font.DemiBold
        }

        // Résolution terrain
        Column {
            width: parent.width
            spacing: 4
            Text {
                text: "Résolution (densité): " + terrainManager.resolution
                color: "white"
                font.pixelSize: 13
            }
            Slider {
                width: parent.width
                from: 2
                to: 1024
                stepSize: 1
                value: terrainManager.resolution
                onValueChanged: terrainManager.resolution = Math.round(value)
                onPressedChanged: if (!pressed) terrainManager.generate()
            }
        }

        // Résolution heightmap
        Column {
            width: parent.width
            spacing: 4
            Text {
                text: "Résolution heightmap: " + terrainManager.heightmapResolution
                color: "white"
                font.pixelSize: 13
            }
            ComboBox {
                width: Math.min(160, parent.width)
                model: [512, 1024, 2048, 4096]
                currentIndex: model.indexOf(terrainManager.heightmapResolution)
                onActivated: terrainManager.heightmapResolution = parseInt(currentText)
            }
        }
    }
}

