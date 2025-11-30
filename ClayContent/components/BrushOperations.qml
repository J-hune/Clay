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
                {op: 0, label: "Raise"},
                {op: 1, label: "Lower"},
                {op: 2, label: "Smooth"},
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

                contentItem: Text {
                    text: modelData.label
                    color: opButton.isActive ? "#000000" : "#d3d6da"
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                onClicked: {
                    brushManager.brushOperation = modelData.op
                }
            }
        }
    }
}
