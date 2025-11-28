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
        spacing: 16

        // Header section
        Column {
            width: parent.width
            spacing: 6

            Text {
                text: "Pinceaux"
                color: "#e5e9f0"
                font.pixelSize: 15
                font.weight: Font.DemiBold
            }

            Rectangle {
                width: parent.width
                height: 1
                color: "#3a3d42"
            }
        }

        Row {
            width: parent.width
            spacing: 20

            // Grande prévisualisation
            Rectangle {
                width: 108
                height: 108
                color: "#1e2024"
                radius: 12
                border.color: "transparent"
                border.width: 2

                property string currentBrushPath: brushManager.brushCount > 0 ?
                    brushManager.brushModel.data(brushManager.brushModel.index(brushManager.brushIndex, 0), 259) : ""

                Image {
                    id: currentBrushImage
                    anchors.fill: parent
                    anchors.margins: 2
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
                        radius: 12
                        color: "black"
                    }
                }
            }

            // Contrôles
            Column {
                width: parent.width - 108 - 20
                spacing: 10

                Text {
                    text: "Paramètres"
                    color: "#d8dee9"
                    font.pixelSize: 13
                    font.weight: Font.Medium
                }

                CustomSlider {
                    id: brushSizeSlider
                    width: parent.width
                    from: 0.1
                    to: 10
                    stepSize: 0.1
                    value: brushManager.brushSize
                    label: "Taille"
                    decimals: 1
                    primaryColor: "#5e81ac"
                    textColor: "#d8dee9"
                    onValueChanged: brushManager.brushSize = value
                }

                CustomSlider {
                    id: brushStrengthSlider
                    width: parent.width
                    from: 0.0
                    to: 5.0
                    stepSize: 0.05
                    value: brushManager.brushStrength
                    label: "Intensité"
                    decimals: 2
                    primaryColor: "#5e81ac"
                    textColor: "#d8dee9"
                    onValueChanged: brushManager.brushStrength = value
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "#3a3d42"
                }

                BrushOperations {
                    width: parent.width
                    brushManager: brushSection.parent.brushManager
                }
            }
        }

        // Séparateur
        Rectangle {
            width: parent.width
            height: 1
            color: "#3a3d42"
        }

        // Galerie de brushes
        Column {
            width: parent.width
            spacing: 10

            Text {
                text: "Galerie (" + brushManager.brushCount + " pinceaux)"
                color: "#d8dee9"
                font.pixelSize: 13
                font.weight: Font.Medium
            }

            // Grille de brushes
            Grid {
                id: brushGrid
                width: parent.width
                property int cellSize: 68
                property int cellSpacing: 8
                columns: Math.max(1, Math.floor((width + cellSpacing) / (cellSize + cellSpacing)))
                spacing: cellSpacing
                rowSpacing: cellSpacing

                Repeater {
                    id: brushRepeater
                    model: brushManager.brushCount

                    Rectangle {
                        width: brushGrid.cellSize
                        height: brushGrid.cellSize
                        color: brushManager.brushIndex === index ? "#2e3440" : "#1e2024"
                        radius: 10
                        border.color: brushManager.brushIndex === index ? "#5e81ac" : "#3a3d42"
                        border.width: brushManager.brushIndex === index ? 2 : 1

                        Behavior on color { ColorAnimation { duration: 150 } }

                        property string brushFilePath: brushManager.brushModel.data(brushManager.brushModel.index(index, 0), 259)
                        property string brushName: brushManager.brushModel.data(brushManager.brushModel.index(index, 0), 258)

                        Image {
                            id: brushImage
                            anchors.fill: parent
                            anchors.margins: 8
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
                            opacity: brushMouseArea.containsMouse ? 1.0 : 0.85
                            Behavior on opacity { NumberAnimation { duration: 100 } }
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
                                radius: 8
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
}

