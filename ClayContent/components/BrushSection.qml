import QtQuick
import QtQuick.Controls
import QtQuick.Effects

Rectangle {
    width: parent.width
    height: brushSection.height
    color: "transparent"

    required property var brushManager

    Column {
        id: brushSection
        width: parent.width
        spacing: 8

        // Header section
        Row {
            width: parent.width
            spacing: 8
            Text {
                text: "Brush"
                color: "#bf5934"
                font.pixelSize: 16
                font.weight: Font.DemiBold
            }
            Text {
                anchors.bottom: parent.bottom
                text: brushManager.brushCount > 0 ?
                    brushManager.brushModel.data(brushManager.brushModel.index(brushManager.brushIndex, 0), 258) :
                    "None"
                color: "#aaa"
                font.pixelSize: 12
                font.italic: true
            }
        }

        // Prévisualisation du brush actuel avec contrôles
        Row {
            width: parent.width
            spacing: 12
            bottomPadding: 4

            // Prévisualisation 128px
            Rectangle {
                width: 120
                height: 120
                color: "#2e3136"
                radius: 8
                border.color: "#52555b"
                border.width: 2

                property string currentBrushPath: brushManager.brushCount > 0 ?
                    brushManager.brushModel.data(brushManager.brushModel.index(brushManager.brushIndex, 0), 259) : ""

                Image {
                    id: currentBrushImage
                    anchors.fill: parent
                    anchors.margins: 8
                    source: parent.currentBrushPath ? "file://" + parent.currentBrushPath : ""
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    visible: false
                }

                MultiEffect {
                    source: currentBrushImage
                    anchors.fill: currentBrushImage
                    maskEnabled: true
                    maskSource: currentBrushMask
                }

                Item {
                    id: currentBrushMask
                    width: currentBrushImage.width
                    height: currentBrushImage.height
                    layer.enabled: true
                    visible: false

                    Rectangle {
                        width: currentBrushImage.width
                        height: currentBrushImage.height
                        radius: 8
                        color: "black"
                    }
                }
            }

            // Contrôles (sliders)
            Column {
                width: parent.width - 140
                spacing: 12
                anchors.verticalCenter: parent.verticalCenter

                // Brush size
                CustomSlider {
                    id: brushSizeSlider
                    width: parent.width
                    from: 0.1
                    to: 10
                    stepSize: 0.1
                    value: brushManager.brushSize
                    label: "Taille"
                    decimals: 1
                    onValueChanged: brushManager.brushSize = value
                }

                // Brush strength
                CustomSlider {
                    id: brushStrengthSlider
                    width: parent.width
                    from: 0.0
                    to: 5.0
                    stepSize: 0.05
                    value: brushManager.brushStrength
                    label: "Intensité"
                    decimals: 2
                    onValueChanged: brushManager.brushStrength = value
                }
            }
        }

        // Grille de brushes responsive
        Flow {
            id: brushFlow
            width: parent.width
            spacing: 6

            Repeater {
                id: brushRepeater
                model: brushManager.brushCount

                Rectangle {
                    width: 64
                    height: 64
                    color: brushManager.brushIndex === index ? "#bf5934" : "#2e3136"
                    radius: 6
                    border.color: brushManager.brushIndex === index ? "#ff8855" : "#52555b"
                    border.width: brushManager.brushIndex === index ? 2 : 1

                    property string brushFilePath: brushManager.brushModel.data(brushManager.brushModel.index(index, 0), 259)
                    property string brushName: brushManager.brushModel.data(brushManager.brushModel.index(index, 0), 258)

                    Image {
                        id: brushImage
                        anchors.fill: parent
                        anchors.margins: 5
                        source: "file://" + parent.brushFilePath
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        visible: false
                    }

                    MultiEffect {
                        source: brushImage
                        anchors.fill: brushImage
                        maskEnabled: true
                        maskSource: mask
                    }

                    Item {
                        id: mask
                        width: brushImage.width
                        height: brushImage.height
                        layer.enabled: true
                        visible: false

                        Rectangle {
                            width: brushImage.width
                            height: brushImage.height
                            radius: 6
                            color: "black"
                        }
                    }

                    MouseArea {
                        id: brushMouseArea
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        hoverEnabled: true
                        onClicked: brushManager.brushIndex = index
                    }
                }
            }
        }
    }
}

