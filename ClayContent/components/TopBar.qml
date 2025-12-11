import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Rectangle {
    id: topBar
    height: 60
    color: "#2e3136"
    border.width: 0

    required property var terrainManager
    required property var glViewport
    required property var erosionController

    property bool terrainPopupVisible: false
    property bool erosionPopupVisible: false

    // Alias pour compatibilité
    readonly property var terrainPopup: QtObject {
        property bool visible: terrainPopupVisible
    }

    // Alias pour exposer les éléments internes
    property alias erosionButtonItem: erosionButton

    Row {
        id: topRow
        anchors.fill: parent
        anchors.margins: 8
        spacing: 12

        // Bouton Terrain stylisé
        Rectangle {
            id: terrainButton
            width: terrainText.paintedWidth + 60
            height: 40
            radius: 22
            anchors.verticalCenter: parent.verticalCenter
            color: terrainMouseArea.pressed ? "#4a4d52" : (terrainMouseArea.containsMouse ? "#4c566a" : "#3a3d42")
            border.width: 0

            Behavior on color {
                ColorAnimation { duration: 100 }
            }

            Row {
                anchors.centerIn: parent
                spacing: 8

                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    sourceSize.width: 18
                    sourceSize.height: 18
                    source: "../images/layer-plus.svg"
                    fillMode: Image.PreserveAspectFit
                    opacity: terrainMouseArea.containsMouse ? 1.0 : 0.7
                }

                Text {
                    id: terrainText
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Terrain"
                    color: "#e5e9f0"
                    font.pixelSize: 14
                    font.weight: Font.Medium
                }
            }

            MouseArea {
                id: terrainMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: terrainPopupVisible = !terrainPopupVisible
            }
        }

        // Bouton Érosion stylisé
        Rectangle {
            id: erosionButton
            width: erosionText.paintedWidth + 60
            height: 40
            radius: 22
            anchors.verticalCenter: parent.verticalCenter
            color: erosionMouseArea.pressed ? "#4a4d52" : (erosionMouseArea.containsMouse ? "#4c566a" : "#3a3d42")
            border.width: 0

            Behavior on color {
                ColorAnimation {
                    duration: 100
                }
            }

            Row {
                anchors.centerIn: parent
                spacing: 8

                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    sourceSize.width: 18
                    sourceSize.height: 18
                    source: "../images/erode_settings.svg"
                    fillMode: Image.PreserveAspectFit
                    opacity: erosionMouseArea.containsMouse ? 1.0 : 0.7
                }

                Text {
                    id: erosionText
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Érosion"
                    color: "#e5e9f0"
                    font.pixelSize: 14
                    font.weight: Font.Medium
                }
            }

            MouseArea {
                id: erosionMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: erosionPopupVisible = !erosionPopupVisible
            }
        }

        // Bouton Export Heightmap
        Rectangle {
            id: exportButton
            width: exportText.paintedWidth + 60
            height: 40
            radius: 22
            anchors.verticalCenter: parent.verticalCenter
            color: exportMouseArea.pressed ? "#4a4d52" : (exportMouseArea.containsMouse ? "#5e81ac" : "#4c566a")
            border.width: 0
            enabled: terrainManager.ready
            opacity: enabled ? 1.0 : 0.4

            Behavior on color {
                ColorAnimation { duration: 100 }
            }

            Row {
                anchors.centerIn: parent
                spacing: 8

                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    sourceSize.width: 16
                    sourceSize.height: 16
                    source: "../images/arrow-down-to-bracket.svg"
                    fillMode: Image.PreserveAspectFit
                    opacity: exportMouseArea.containsMouse ? 1.0 : 0.7
                }

                Text {
                    id: exportText
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Exporter"
                    color: "#e5e9f0"
                    font.pixelSize: 14
                    font.weight: Font.Medium
                }
            }

            MouseArea {
                id: exportMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                enabled: parent.enabled
                onClicked: exportDialog.open()
            }
        }
    }

    // Dialog pour exporter la heightmap
    FileDialog {
        id: exportDialog
        fileMode: FileDialog.SaveFile
        title: "Exporter Heightmap (PNG 16-bit)"
        nameFilters: ["Images PNG (*.png)"]
        defaultSuffix: "png"
        currentFolder: "file://" + Qt.resolvedUrl(".").toString().replace("file://", "") + "../../"

        onAccepted: {
            var filePath = selectedFile.toString()
            glViewport.exportHeightmap(filePath)
        }
    }

    // Popup moderne pour le terrain
    Loader {
        id: popupLoader
        active: terrainPopupVisible

        sourceComponent: TerrainPopup {
            parent: topBar.Window ? topBar.Window.contentItem : topBar.parent
            terrainManager: topBar.terrainManager
            visible: terrainPopupVisible

            x: {
                if (!parent) return 0
                var buttonPos = terrainButton.mapToItem(parent, 0, 0)
                return buttonPos.x
            }
            y: {
                if (!parent) return 0
                var buttonPos = terrainButton.mapToItem(parent, 0, 0)
                return buttonPos.y + terrainButton.height + 8
            }
        }
    }

    // Popup pour l'érosion
    Loader {
        id: erosionPopupLoader
        active: erosionPopupVisible

        property var _erosionController: topBar.erosionController
        property var _erosionButton: erosionButton
        property var _topBar: topBar
        property var _glViewport: topBar.glViewport
        property bool _erosionPopupVisible: topBar.erosionPopupVisible

        sourceComponent: ErosionPopup {
            erosionController: erosionPopupLoader._erosionController
            erosionButton: erosionPopupLoader._erosionButton
            topBar: erosionPopupLoader._topBar
            glViewport: erosionPopupLoader._glViewport
            erosionPopupVisible: erosionPopupLoader._erosionPopupVisible
        }
    }

    // Overlay pour détecter les clics en dehors du popup terrain
    Loader {
        id: overlayLoader
        active: terrainPopupVisible
        sourceComponent: Item {
            parent: Overlay.overlay || topBar.Window.contentItem || topBar.parent
            anchors.fill: parent
            z: 998

            MouseArea {
                acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
                anchors.fill: parent
                onPressed: (mouse) => {
                    // Récupérer le popup depuis le loader
                    var popup = popupLoader.item
                    if (!popup) {
                        mouse.accepted = false
                        return
                    }

                    // Vérifier si le clic est en dehors du popup et du bouton terrain
                    var popupPos = mapToItem(popup, mouse.x, mouse.y)
                    var buttonPos = mapToItem(terrainButton, mouse.x, mouse.y)

                    var outsidePopup = popupPos.x < 0 || popupPos.x > popup.width || popupPos.y < 0 || popupPos.y > popup.height
                    var outsideButton = buttonPos.x < 0 || buttonPos.x > terrainButton.width || buttonPos.y < 0 || buttonPos.y > terrainButton.height

                    if (outsidePopup && outsideButton) {
                        terrainPopupVisible = false
                    }
                    mouse.accepted = false
                }
            }
        }
    }

    // Overlay pour détecter les clics en dehors du popup érosion
    Loader {
        id: erosionOverlayLoader
        active: erosionPopupVisible
        sourceComponent: MouseArea {
            acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
            parent: Overlay.overlay || topBar.Window.contentItem || topBar.parent
            anchors.fill: parent
            z: 9999

            onPressed: (mouse) => {
                // Vérifier si le clic est en dehors du bouton érosion
                var buttonPos = erosionButton.mapToItem(parent, 0, 0)
                var clickInButton = mouse.x >= buttonPos.x && mouse.x <= buttonPos.x + erosionButton.width &&
                    mouse.y >= buttonPos.y && mouse.y <= buttonPos.y + erosionButton.height

                // Si le clic n'est pas dans le bouton, fermer le popup
                // Le popup lui-même bloque ses propres clics avec son MouseArea
                if (!clickInButton) {
                    erosionPopupVisible = false
                    if (erosionController && erosionController.isErosionRunning) {
                        erosionController.toggleErosion();
                        // Si on vient d'arrêter l'érosion -> capture snapshot APRÈS
                        if (!erosionController.isErosionRunning) {
                            glViewport.captureSnapshot()
                        }
                    }
                }
                mouse.accepted = false
            }
        }
    }
}

