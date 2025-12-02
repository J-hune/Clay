import QtQuick
import QtQuick.Controls

Column {
    id: meshSettings
    spacing: 10
    
    required property var terrainManager

    Text {
        text: "Paramètres du mesh"
        color: "#d8dee9"
        font.pixelSize: 13
        font.weight: Font.Medium
    }

    CustomSlider {
        width: parent.width
        from: 2
        to: 1024
        stepSize: 1
        value: terrainManager.resolution
        label: "Résolution (densité)"
        decimals: 0
        primaryColor: "#5e81ac"
        textColor: "#d8dee9"
        onValueChanged: terrainManager.resolution = Math.round(value)
    }

    CustomSlider {
        width: parent.width
        from: 0
        to: 200
        stepSize: 1
        value: terrainManager.heightScale
        label: "Échelle hauteur"
        decimals: 0
        primaryColor: "#5e81ac"
        textColor: "#d8dee9"
        onValueChanged: terrainManager.heightScale = value
    }

    Row {
        width: parent.width
        spacing: 8

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: "Résolution heightmap:"
            color: "#a0a0a0"
            font.pixelSize: 12
        }

        ComboBox {
            id: heightmapResCombo
            width: 120
            model: [512, 1024, 2048, 4096]
            currentIndex: model.indexOf(terrainManager.heightmapResolution)
            onActivated: terrainManager.heightmapResolution = parseInt(currentText)

            background: Rectangle {
                color: heightmapResCombo.pressed ? "#3a3d42" : "#212429"
                radius: 6
                border.width: 1
                border.color: "#3a3d42"
            }

            contentItem: Text {
                text: heightmapResCombo.displayText
                color: "#e5e9f0"
                font.pixelSize: 12
                verticalAlignment: Text.AlignVCenter
                leftPadding: 10
            }

            popup: Popup {
                y: heightmapResCombo.height
                width: heightmapResCombo.width
                implicitHeight: contentItem.implicitHeight
                padding: 1
                z: 20000

                contentItem: ListView {
                    clip: true
                    implicitHeight: contentHeight
                    model: heightmapResCombo.popup.visible ? heightmapResCombo.delegateModel : null
                    currentIndex: heightmapResCombo.highlightedIndex

                    ScrollIndicator.vertical: ScrollIndicator {}
                }

                background: Rectangle {
                    color: "#272a2f"
                    border.color: "#4c566a"
                    border.width: 1
                    radius: 6
                }
            }

            delegate: ItemDelegate {
                width: heightmapResCombo.width
                contentItem: Text {
                    text: modelData
                    color: "#e5e9f0"
                    font.pixelSize: 12
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: highlighted ? "#4c566a" : (hovered ? "#3a3d42" : "transparent")
                    radius: 4
                }
                highlighted: heightmapResCombo.highlightedIndex === index
                hoverEnabled: true
            }
        }
    }
}
