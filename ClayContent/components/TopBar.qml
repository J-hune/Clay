import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

Rectangle {
    id: topBar
    height: 60
    color: "#2e3136"
    border.width: 0

    required property var terrainManager

    Row {
        id: topRow
        anchors.fill: parent
        anchors.margins: 8
        spacing: 12

        Button {
            id: terrainButton
            anchors.verticalCenter: parent.verticalCenter
            text: "Terrain"
            icon.source: "../images/layer-plus.svg"
            icon.width: 16
            icon.height: 16
            icon.color: "white"
            onClicked: terrainPopup.open()
        }
    }

    Popup {
        id: terrainPopup
        x: terrainButton.x
        y: topBar.height
        width: 300
        modal: false
        focus: true
        padding: 12

        contentItem: Column {
            spacing: 8

            Text {
                text: "Génération Terrain"
                font.pixelSize: 16
                color: "white"
            }

            Rectangle {
                height: 1
                width: parent.width
                color: "#333"
            }

            Row {
                spacing: 6
                Text {
                    text: "Mode:"
                    color: "white"
                }
                ComboBox {
                    id: modeCombo
                    model: ["Plat", "Heightmap"]
                    currentIndex: terrainManager.mode
                    onCurrentIndexChanged: terrainManager.mode = currentIndex
                    width: 120
                }
            }

            Row {
                spacing: 6
                Text {
                    text: "HeightScale:"
                    color: "white"
                }
                Slider {
                    id: heightScaleSlider
                    from: 0
                    to: 200
                    stepSize: 1
                    value: terrainManager.heightScale
                    onValueChanged: terrainManager.heightScale = value
                    width: 120
                }
                Text {
                    text: Math.round(terrainManager.heightScale)
                    color: "#ccc"
                }
            }

            Row {
                spacing: 6
                visible: terrainManager.mode === 1
                Button {
                    text: "Image..."
                    onClicked: fileDialog.open()
                }
                Text {
                    text: terrainManager.heightmapSource.toString().length > 0 ? terrainManager.heightmapSource.toString().split('/').pop() : "(Aucune)"
                    color: "#ccc"
                    elide: Text.ElideRight
                    width: 110
                }
            }

            FileDialog {
                id: fileDialog
                title: "Choisir une image heightmap"
                onAccepted: {
                    terrainManager.heightmapSource = fileDialog.selectedFile
                }
            }

            Button {
                text: terrainManager.ready ? "Régénérer" : "Générer"
                onClicked: terrainManager.generate()
            }

            Text {
                text: terrainManager.ready ? ("Triangles: " + terrainManager.triangleCount) : "Terrain non prêt"
                color: "#ccc"
                font.pixelSize: 12
            }

            Button {
                text: "Fermer"
                onClicked: terrainPopup.close()
            }
        }

        background: Rectangle {
            radius: 6
            color: "#212429"
            border.color: "#444"
            border.width: 1
        }
    }
}


