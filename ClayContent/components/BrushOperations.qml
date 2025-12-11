import QtQuick
import QtQuick.Controls

Column {
    id: brushOpsSection
    spacing: 8

    required property var brushManager
    required property var glView

    Text {
        text: "Operations"
        color: "#d8dee9"
        font.pixelSize: 13
        font.weight: Font.Medium
    }

    // Première ligne : Raise, Lower, Smooth
    Row {
        spacing: 6
        width: parent.width

        Repeater {
            model: [
                {op: 0, label: "Raise", icon: "raise.svg"},
                {op: 1, label: "Lower", icon: "lower.svg"},
                {op: 2, label: "Smooth", icon: "smooth.svg"}
            ]

            Button {
                id: opButton
                width: (parent.width - 6 * (3 - 1)) / 3
                height: 50

                property bool isActive: brushManager.brushOperation === modelData.op

                background: Rectangle {
                    color: opButton.isActive ? "#DBB1BC" : (opButton.hovered ? "#3A3D42" : "#1e2024")
                    radius: 6

                    Behavior on color {
                        ColorAnimation {
                            duration: 100
                        }
                    }
                }

                icon.source: "../images/" + modelData.icon
                icon.width: Math.min(20, width)
                icon.height: Math.min(24, height)
                icon.color: opButton.isActive ? "#2E2B3C" : "#d8dee9"

                onClicked: {
                    brushManager.brushOperation = modelData.op
                    // Mode terrain (pas d'édition de masque)
                    if (glView) {
                        glView.erosionUiActive = false
                    }
                }
            }
        }
    }

    // Deuxième ligne : Masque d'érosion
    Text {
        text: "Masque d'érosion"
        color: "#d8dee9"
        font.pixelSize: 13
        font.weight: Font.Medium
        topPadding: 8
    }

    Row {
        spacing: 6
        width: parent.width

        Repeater {
            model: [
                {op: 3, label: "Add", icon: "erosion_add.svg"},
                {op: 4, label: "Erase", icon: "erosion_erase.svg"}
            ]

            Button {
                id: erosionButton
                width: (parent.width - 6) / 2
                height: 50

                property bool isActive: brushManager.brushOperation === modelData.op

                background: Rectangle {
                    color: erosionButton.isActive ? "#88C0D0" : (erosionButton.hovered ? "#3A3D42" : "#1e2024")
                    radius: 6

                    Behavior on color {
                        ColorAnimation {
                            duration: 100
                        }
                    }
                }

                icon.source: "../images/" + modelData.icon
                icon.width: Math.min(20, width)
                icon.height: Math.min(24, height)
                icon.color: erosionButton.isActive ? "#2E2B3C" : "#d8dee9"

                onClicked: {
                    brushManager.brushOperation = modelData.op
                    // Mode édition de masque d'érosion
                    if (glView) {
                        glView.erosionUiActive = true
                    }
                }
            }
        }
    }
}
