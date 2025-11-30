import QtQuick
import QtQuick.Controls

Column {
    id: brushOpsSection
    spacing: 8

    required property var brushManager

    Text {
        text: "Operations"
        color: "#d8dee9"
        font.pixelSize: 13
        font.weight: Font.Medium
    }

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
                }
            }
        }
    }
}
