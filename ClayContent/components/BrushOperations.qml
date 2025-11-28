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
                {op: 0, label: "Raise", tooltip: "Élever le terrain"},
                {op: 1, label: "Lower", tooltip: "Abaisser le terrain"},
                {op: 2, label: "Smooth", tooltip: "Lisser le terrain"},
            ]

            Button {
                id: opButton
                width: (parent.width - 6 * (3 - 1)) / 3
                height: 50

                property bool isActive: brushManager.brushOperation === modelData.op

                background: Rectangle {
                    color: opButton.isActive ? "#bf5934" : (opButton.hovered ? "#33353a" : "#2a2d32")
                    border.color: opButton.isActive ? "#d96b44" : "#3f4248"
                    border.width: 1
                    radius: 4

                    Behavior on color {
                        ColorAnimation {
                            duration: 100
                        }
                    }
                }

                contentItem: Text {
                    text: modelData.label
                    color: opButton.isActive ? "#ffffff" : "#d3d6da"
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                ToolTip.visible: hovered
                ToolTip.text: modelData.tooltip
                ToolTip.delay: 500

                onClicked: {
                    brushManager.brushOperation = modelData.op
                }
            }
        }
    }
}
