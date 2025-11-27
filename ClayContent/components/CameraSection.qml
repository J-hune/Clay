import QtQuick
import QtQuick.Controls

Rectangle {
    width: parent.width
    height: cameraSection.height
    color: "transparent"

    required property var cameraController
    required property var glView

    property real initialYaw: -90
    property real initialPitch: 0
    property real initialMouseSensitivity: 0.15
    property int initialGridResolution: 50

    Column {
        id: cameraSection
        width: parent.width
        spacing: 8

        // Header section
        Text {
            text: "Caméra & Affichage"
            color: "#bf5934"
            font.pixelSize: 16
            font.weight: Font.DemiBold
        }

        // Sensibilité souris
        Column {
            width: parent.width
            spacing: 4
            Text {
                text: "Sensibilité souris: " + cameraController.mouseSensitivity.toFixed(2)
                color: "white"
                font.pixelSize: 13
            }
            Row {
                width: parent.width
                spacing: 6
                Slider {
                    id: sensSlider
                    from: 0
                    to: 1
                    stepSize: 0.01
                    value: cameraController.mouseSensitivity
                    onValueChanged: cameraController.mouseSensitivity = value
                    width: parent.width - 40
                }
                Button {
                    width: 32
                    height: 32
                    icon.height: 16
                    icon.width: 16
                    icon.source: "../images/arrow-rotate-left.svg"
                    onClicked: cameraController.mouseSensitivity = parent.parent.parent.parent.initialMouseSensitivity
                }
            }
        }

        // Résolution grille
        Column {
            width: parent.width
            spacing: 4
            Text {
                text: "Résolution grille: " + glView.gridResolution
                color: "white"
                font.pixelSize: 13
            }
            Row {
                width: parent.width
                spacing: 6
                Slider {
                    id: gridSlider
                    from: 1
                    to: 500
                    stepSize: 1
                    value: glView.gridResolution
                    onValueChanged: glView.gridResolution = Math.round(value)
                    width: parent.width - 40
                }
                Button {
                    width: 32
                    height: 32
                    icon.source: "../images/arrow-rotate-left.svg"
                    icon.height: 16
                    icon.width: 16
                    onClicked: glView.gridResolution = parent.parent.parent.parent.initialGridResolution
                }
            }
        }

        // Switches grille et axes
        Row {
            spacing: Math.max(12, (parent.width - 200) / 2)
            Switch {
                id: gridSwitch
                text: "Grille"
                checked: glView.drawGrid
                Material.accent: "#bf5934"
                onToggled: {
                    glView.drawGrid = checked
                    glView.update()
                }
            }
            Switch {
                id: axesSwitch
                text: "Axes"
                checked: glView.drawAxes
                Material.accent: "#bf5934"
                onToggled: {
                    glView.drawAxes = checked
                    glView.update()
                }
            }
        }
    }
}

