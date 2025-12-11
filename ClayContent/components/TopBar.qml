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

        sourceComponent: Rectangle {
            id: erosionPopupContent
            parent: Overlay.overlay || topBar.Window.contentItem || topBar.parent
            width: 900
            height: Math.min(2800, erosionControlContent.implicitHeight + 40)
            color: "#272a2f"
            radius: 10
            border.width: 1
            border.color: "#3a3d42"
            x: {
                var buttonGlobal = erosionButton.mapToItem(null, 0, 0)
                return buttonGlobal ? buttonGlobal.x : 8
            }
            y: {
                var buttonGlobal = erosionButton.mapToItem(null, 0, 0)
                return buttonGlobal ? buttonGlobal.y + erosionButton.height + 8 : topBar.height + 8
            }
            z: 10000
            visible: erosionPopupVisible

            // Timer pour l'érosion continue
            Timer {
                id: erosionTimer
                interval: 10 // 0.1 secondes
                repeat: true
                running: erosionController ? erosionController.isErosionRunning : false
                onTriggered: {
                    if (erosionController) {
                        erosionController.applyErosion()
                    }
                }
            }

            // Ombre portée
            layer.enabled: true
            layer.effect: ShaderEffect {
                property color shadowColor: "#cc000000"
            }

            // MouseArea pour bloquer les clics et empêcher la fermeture du popup
            MouseArea {
                anchors.fill: parent
                onPressed: (mouse) => {
                    mouse.accepted = true
                }
                onReleased: (mouse) => {
                    mouse.accepted = true
                }
                onClicked: (mouse) => {
                    mouse.accepted = true
                }
            }

            // Triangle indicateur
            Canvas {
                id: erosionTriangleIndicator
                width: 16
                height: 8
                anchors.bottom: parent.top
                anchors.left: parent.left
                anchors.leftMargin: 40
                anchors.bottomMargin: -1

                onPaint: {
                    var ctx = getContext("2d");
                    ctx.reset();
                    ctx.fillStyle = "#272a2f";
                    ctx.strokeStyle = "#3a3d42";
                    ctx.lineWidth = 1;

                    ctx.beginPath();
                    ctx.moveTo(width / 2, 0);
                    ctx.lineTo(width, height);
                    ctx.lineTo(0, height);
                    ctx.closePath();
                    ctx.fill();
                    ctx.stroke();
                }
            }

            // Contenu du contrôle d'érosion
            Item {
                id: erosionControlContent
                implicitHeight: contentColumnErosion.implicitHeight + 20
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 20

                Column {
                    id : contentColumnErosion
                    width: parent.width
                    spacing: 12
                    RowLayout {
                        width: parent.width
                        // alignements verticaux via Layout
                        Text {
                            Layout.alignment: Qt.AlignVCenter
                            text: "Paramètres d'érosion hydraulique"
                            color: "#e5e9f0"
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                        }

                        // spacer flexible qui pousse le bouton à droite
                        Item { Layout.fillWidth: true }
                        // Bouton pour appliquer l'érosion

                        Rectangle {
                            width: 160
                            height: 40
                            radius: 6
                            color: {
                                if (erosionController && erosionController.isErosionRunning) {
                                    return generateMouseAreaErosion.pressed ? "#bf616a" : (generateMouseAreaErosion.containsMouse ? "#d08770" : "#d08770")
                                } else {
                                    return generateMouseAreaErosion.pressed ? "#5e81ac" : (generateMouseAreaErosion.containsMouse ? "#4c72a0" : "#4c566a")
                                }
                            }
                            border.width: 0

                            Behavior on color {
                                ColorAnimation {
                                    duration: 100
                                }
                            }

                            Text {
                                anchors.centerIn: parent
                                text: erosionController && erosionController.isErosionRunning ? "Arrêter érosion" : "Appliquer l'érosion"
                                color: "#eceff4"
                                font.pixelSize: 13
                                font.weight: Font.DemiBold
                            }

                            MouseArea {
                                id: generateMouseAreaErosion
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (erosionController) {
                                        erosionController.toggleErosion()
                                    }
                                }
                            }
                        }
                    }


                    // Séparateur
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#3a3d42"
                    }

                    // Contrôles d'érosion en grille
                    Flow {
                        width: contentColumnErosion.width
                        spacing: 12
                        flow: Flow.LeftToRight




                        // Particules
                        CustomSlider {
                            label: "Particules"
                            from: 1000
                            to: 70000
                            stepSize: 100
                            decimals: 0
                            primaryColor: "#5e81ac"
                            textColor: "#d8dee9"
                            value: erosionController ? erosionController.numParticles : 1000
                            onValueChanged: if (erosionController) erosionController.numParticles = value
                            width: 200
                        }

                        // Vitesse d'érosion
                        CustomSlider {
                            label: "Vitesse d'érosion"
                            from: 0.0
                            to: 0.4
                            stepSize: 0.01
                            decimals: 2
                            primaryColor: "#5e81ac"
                            textColor: "#d8dee9"
                            value: erosionController ? erosionController.erosionSpeed : 0.3
                            onValueChanged: if (erosionController) erosionController.erosionSpeed = value
                            width: 200
                        }

                        // Capacité sédiment
                        CustomSlider {
                            label: "Capacité sédiment"
                            from: 0.0
                            to: 32.0
                            stepSize: 0.1
                            decimals: 1
                            primaryColor: "#5e81ac"
                            textColor: "#d8dee9"
                            value: erosionController ? erosionController.sedimentCapacity : 8.0
                            onValueChanged: if (erosionController) erosionController.sedimentCapacity = value
                            width: 200
                        }

                        // Dépôt
                        CustomSlider {
                            label: "Dépôt"
                            from: 0.0
                            to: 1.0
                            stepSize: 0.01
                            decimals: 2
                            primaryColor: "#5e81ac"
                            textColor: "#d8dee9"
                            value: erosionController ? erosionController.depositionPercentage : 0.1
                            onValueChanged: if (erosionController) erosionController.depositionPercentage = value
                            width: 200
                        }

                        // Inertie
                        CustomSlider {
                            label: "Inertie"
                            from: 0.1
                            to: 1.0
                            stepSize: 0.01
                            decimals: 2
                            primaryColor: "#5e81ac"
                            textColor: "#d8dee9"
                            value: erosionController ? erosionController.inertia : 0.1
                            onValueChanged: if (erosionController) erosionController.inertia = value
                            width: 200
                        }

                        // Évaporation
                        CustomSlider {
                            label: "Évaporation"
                            from: 0.01
                            to: 0.5
                            stepSize: 0.001
                            decimals: 3
                            primaryColor: "#5e81ac"
                            textColor: "#d8dee9"
                            value: erosionController ? erosionController.evaporationSpeed : 0.01
                            onValueChanged: if (erosionController) erosionController.evaporationSpeed = value
                            width: 200
                        }

                        // Pente minimale
                        CustomSlider {
                            label: "Pente minimale"
                            from: 0.0001
                            to: 0.05
                            stepSize: 0.0001
                            decimals: 4
                            primaryColor: "#5e81ac"
                            textColor: "#d8dee9"
                            value: erosionController ? erosionController.minSlope : 0.001
                            onValueChanged: if (erosionController) erosionController.minSlope = value
                            width: 200
                        }

                        // Rayon d'érosion
                        CustomSlider {
                            label: "Rayon d'érosion"
                            from: 0
                            to: 6
                            stepSize: 1
                            decimals: 0
                            primaryColor: "#5e81ac"
                            textColor: "#d8dee9"
                            value: erosionController ? erosionController.erosionRadius : 1
                            onValueChanged: if (erosionController) erosionController.erosionRadius = value
                            width: 200
                        }

                        // Durée de vie max
                        /*CustomSlider {
                            label: "Durée de vie max"
                            from: 1
                            to: 200
                            stepSize: 1
                            decimals: 0
                            primaryColor: "#5e81ac"
                            textColor: "#d8dee9"
                            value: erosionController ? erosionController.maxLifetime : 30
                            onValueChanged: if (erosionController) erosionController.maxLifetime = value
                            width: 200
                        }*/

                        // Gravité
                        /*CustomSlider {
                            label: "Gravité"
                            from: 0.0
                            to: 10.0
                            stepSize: 0.1
                            decimals: 1
                            primaryColor: "#5e81ac"
                            textColor: "#d8dee9"
                            value: erosionController ? erosionController.gravity : 4.0
                            onValueChanged: if (erosionController) erosionController.gravity = value
                            width: 200
                        }*/
                    }
                }
            }

            // Animation d'apparition
            scale: visible ? 1.0 : 0.95
            opacity: visible ? 1.0 : 0.0

            Behavior on scale {
                NumberAnimation {
                    duration: 150; easing.type: Easing.OutCubic
                }
            }
            Behavior on opacity {
                NumberAnimation {
                    duration: 150
                }
            }

            // Gestion de la touche Escape
            Keys.onEscapePressed: {
                erosionPopupVisible = false
                if (erosionController && erosionController.isErosionRunning) erosionController.toggleErosion();
            }

            focus: visible
            onVisibleChanged: {
                if (visible) {
                    forceActiveFocus()
                } else if (erosionController && erosionController.isErosionRunning) {
                    erosionController.setIsErosionRunning(false)
                }
            }
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
                    if (erosionController && erosionController.isErosionRunning) erosionController.toggleErosion();
                }
                mouse.accepted = false
            }
        }
    }
}

