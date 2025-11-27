/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Effects
import Clay

// Pour ouvrir le projet sur Qt Design Studio, il faut commenter l'import suivant
// (oui c'est un peu con, mais bon...)
import MyGL 1.0

// Couleurs
// Barre gauche : #272a2f
// Barre du haut et droite : #212429
// Switch accent : #bf5934
// Barre vitesse FPS : #866ab6
// Barre distance orbit : #BF5934
Rectangle {
    id: rectangle
    anchors.fill: parent
    implicitWidth: Constants.width
    implicitHeight: Constants.height

    color: Constants.backgroundColor

    // ========================================
    // Composants autonomes
    // ========================================

    CameraController {
        id: cameraController
        speed: 5.0
        mouseSensitivity: 0.15
        orbitDistance: 10.0

        onSpeedChanged: speedBar.showTemp()
        onOrbitDistanceChanged: distanceBar.showTemp()
    }

    BrushManager {
        id: brushManager
        brushIndex: 0
        brushSize: 2.0
        brushStrength: 1.0
    }

    RaycastController {
        id: raycastController
    }

    TerrainManager {
        id: terrainManager
        resolution: 256
        heightmapResolution: 512
        heightScale: 50.0
        mode: 0 // 0=Flat, 1=Heightmap
    }

    Rectangle {
        id: topBar
        height: 60
        color: "#2e3136"
        border.width: 0
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: 0
        anchors.rightMargin: 0
        anchors.topMargin: 0

        Row {
            id: topRow
            anchors.fill: parent
            anchors.margins: 8
            spacing: 12

            Button {
                id: terrainButton
                anchors.verticalCenter: parent.verticalCenter
                text: "Terrain"
                icon.source: "images/layer-plus.svg"
                icon.width: 16; icon.height: 16
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
                    text: "Génération Terrain"; font.pixelSize: 16; color: "white"
                }
                Rectangle {
                    height: 1; width: parent.width; color: "#333"
                }
                Row {
                    spacing: 6
                    Text {
                        text: "Mode:"; color: "white"
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
                        text: "HeightScale:"; color: "white"
                    }
                    Slider {
                        id: heightScaleSlider
                        from: 0;
                        to: 200; stepSize: 1
                        value: terrainManager.heightScale
                        onValueChanged: terrainManager.heightScale = value
                        width: 120
                    }
                    Text {
                        text: Math.round(terrainManager.heightScale); color: "#ccc"
                    }
                }
                Row {
                    spacing: 6
                    visible: terrainManager.mode === 1
                    Button {
                        text: "Image..."; onClicked: fileDialog.open()
                    }
                    Text {
                        text: terrainManager.heightmapSource.toString().length > 0 ? terrainManager.heightmapSource.toString().split('/').pop() : "(Aucune)"; color: "#ccc"; elide: Text.ElideRight; width: 110
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
                    text: terrainManager.ready ? ("Triangles: " + terrainManager.triangleCount) : "Terrain non prêt"; color: "#ccc"; font.pixelSize: 12
                }
                Button {
                    text: "Fermer"; onClicked: terrainPopup.close()
                }
            }
            background: Rectangle {
                radius: 6; color: "#212429"; border.color: "#444"; border.width: 1
            }
        }
    }

    Rectangle {
        id: openGLArea
        border.width: 0
        anchors.left: parent.left
        anchors.right: rightBar.left
        anchors.top: topBar.bottom
        anchors.bottom: parent.bottom
        anchors.leftMargin: 0
        anchors.rightMargin: 0
        anchors.topMargin: 0
        anchors.bottomMargin: 0

        GLViewport {
            id: glView
            anchors.fill: parent
            focus: true
            activeFocusOnTab: true

            // Liaison avec les composants
            cameraController: cameraController
            brushManager: brushManager
            raycastController: raycastController
            terrainManager: terrainManager
        }

        // Barre vitesse FPS (min=plein, max=vide)
        Rectangle {
            id: speedBar
            visible: false
            width: 12
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.topMargin: 128
            anchors.bottomMargin: 128
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            radius: 4
            color: "#222"
            border.width: 1
            border.color: "#444"
            property real value: cameraController.speed
            property real minVal: cameraController.speedMin
            property real maxVal: cameraController.speedMax
            property int timeoutMs: 2000

            function ratio() {
                // min -> 1, max -> 0
                var span = maxVal - minVal;
                if (span <= 0) return 1;
                return (value - minVal) / span;
            }

            function showTemp() {
                visible = true;
                hideTimer.restart();
            }

            Rectangle {
                id: speedFill
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: parent.height * speedBar.ratio()
                radius: parent.radius
                color: "#866ab6"
            }
            Image {
                id: speedImage
                anchors.top: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.topMargin: 6
                width: 15
                height: 15
                opacity: 0.8
                source: "images/person-running.svg"
                fillMode: Image.PreserveAspectFit
            }
            Timer {
                id:
                    hideTimer; interval: speedBar.timeoutMs; running: false; repeat: false; onTriggered: speedBar.visible = false
            }
            // Mise à jour continue
            onValueChanged: speedFill.height = height * ratio()
        }

        // Barre distance orbit (min=plein, max=vide)
        Rectangle {
            id: distanceBar
            visible: false
            width: 12
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.topMargin: 128
            anchors.bottomMargin: 128
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            radius: 4
            color: "#222"
            border.width: 1
            border.color: "#444"
            property real value: cameraController.orbitDistance
            property real minVal: cameraController.orbitDistanceMin
            property real maxVal: cameraController.orbitDistanceMax
            property int timeoutMs: 2000

            function ratio() {
                var span = maxVal - minVal;
                if (span <= 0) return 1;
                return 1 - (value - minVal) / span;
            }

            function showTemp() {
                visible = true;
                hideTimer2.restart();
            }

            Rectangle {
                id: distanceFill
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: parent.height * distanceBar.ratio()
                radius: parent.radius
                color: "#BF5934"
            }
            Image {
                id: distanceImage
                anchors.top: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.topMargin: 6
                width: 15
                height: 15
                opacity: 0.8
                source: "images/magnifying-glass-plus.svg"
                fillMode: Image.PreserveAspectFit
            }

            Timer {
                id:
                    hideTimer2; interval: distanceBar.timeoutMs; running: false; repeat: false; onTriggered: distanceBar.visible = false
            }
            onValueChanged: distanceFill.height = height * ratio()
        }

        // Overlay d'information sur la caméra / rendu
        Rectangle {
            id: debugOverlay
            z: 10
            width: 240
            color: "#00000088"
            radius: 6
            border.width: 1
            border.color: "#444"
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: 8
            anchors.topMargin: 8

            Column {
                id: infoColumn
                spacing: 4
                anchors.left: parent.left
                anchors.right: parent.right

                Text {
                    text: "FPS: " + (glView.fps > 0 ? glView.fps.toFixed(1) : "--")
                    color: "white"
                    font.pixelSize: 14
                }
                Text {
                    text: "Pos: " + cameraController.position.x.toFixed(1) + ", " + cameraController.position.y.toFixed(1) + ", " + cameraController.position.z.toFixed(1)
                    color: "white"
                    font.pixelSize: 14
                }
                Text {
                    text: "Yaw: " + cameraController.yaw.toFixed(1) + "  Pitch: " + cameraController.pitch.toFixed(1)
                    color: "white"
                    font.pixelSize: 14
                }
            }
        }
    }

    Rectangle {
        id: rightBar
        width: 400
        color: "#272a2f"
        border.width: 0
        anchors.right: parent.right
        anchors.top: topBar.bottom
        anchors.bottom: parent.bottom
        anchors.rightMargin: 0
        anchors.topMargin: 0
        anchors.bottomMargin: 0

        // Valeurs initiales pour les resets
        property real initialYaw: -90
        property real initialPitch: 0
        property real initialMouseSensitivity: 0.15
        property int initialGridResolution: 50
        property real initialCameraSpeed: 5.0

        // Handle gauche pour redimensionner la rightBar
        Rectangle {
            id: rightHandle
            width: 5
            height: parent.height
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            color: "#272a2f"
            Behavior on color {
                ColorAnimation {
                    duration: 100
                }
            }

            MouseArea {
                id: rightDragArea
                anchors.fill: parent
                cursorShape: Qt.SizeHorCursor
                hoverEnabled: true

                property bool dragging: false
                property real startW
                property real startX

                onEntered: rightHandle.color = "#2e3136"
                onExited: {
                    if (!dragging) {
                        rightHandle.color = "#272a2f"
                    }
                }

                onPressed: (m) => {
                    dragging = true
                    startW = rightBar.width
                    startX = mapToGlobal(Qt.point(m.x, m.y)).x
                }

                onPositionChanged: (m) => {
                    if (dragging) {
                        const globalX = mapToGlobal(Qt.point(m.x, m.y)).x;
                        // Tirer vers la gauche => augmente la largeur, vers la droite => la réduit
                        const delta = globalX - startX;
                        const minW = 200;
                        const maxW = 600;
                        rightBar.width = Math.max(minW, Math.min(maxW, startW - delta));
                    }
                }

                onReleased: {
                    dragging = false
                    if (!containsMouse) {
                        rightHandle.color = "#272a2f"
                    }
                }
            }
        }

        ScrollView {
            id: scrollView
            anchors.fill: parent
            anchors.leftMargin: 12
            clip: true

            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            Column {
                id: controlColumn
                width: scrollView.width - scrollView.effectiveScrollBarWidth - 4
                topPadding: 10
                bottomPadding: 10
                spacing: 10

                // ═══════════════════════════════════════════
                // SECTION: CAMÉRA & AFFICHAGE
                // ═══════════════════════════════════════════
                Rectangle {
                    width: parent.width
                    height: cameraSection.height
                    color: "transparent"

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
                                    from: 0;
                                    to: 1; stepSize: 0.01
                                    value: cameraController.mouseSensitivity
                                    onValueChanged: cameraController.mouseSensitivity = value
                                    width: parent.width - 40
                                }
                                Button {
                                    width: 32; height: 32
                                    icon.height: 16; icon.width: 16
                                    icon.source: "images/arrow-rotate-left.svg"
                                    onClicked: cameraController.mouseSensitivity = rightBar.initialMouseSensitivity
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
                                    from: 1;
                                    to: 500; stepSize: 1
                                    value: glView.gridResolution
                                    onValueChanged: glView.gridResolution = Math.round(value)
                                    width: parent.width - 40
                                }
                                Button {
                                    width: 32; height: 32
                                    icon.source: "images/arrow-rotate-left.svg"
                                    icon.height: 16; icon.width: 16
                                    onClicked: glView.gridResolution = rightBar.initialGridResolution
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
                                onToggled: { glView.drawGrid = checked; glView.update(); }
                            }
                            Switch {
                                id: axesSwitch
                                text: "Axes"
                                checked: glView.drawAxes
                                Material.accent: "#bf5934"
                                onToggled: { glView.drawAxes = checked; glView.update(); }
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

                // ═══════════════════════════════════════════
                // SECTION: TERRAIN
                // ═══════════════════════════════════════════
                Rectangle {
                    width: parent.width
                    height: terrainSection.height
                    color: "transparent"

                    Column {
                        id: terrainSection
                        width: parent.width
                        spacing: 8

                        // Header section
                        Text {
                            text: "Terrain"
                            color: "#bf5934"
                            font.pixelSize: 16
                            font.weight: Font.DemiBold
                        }

                        // Résolution terrain
                        Column {
                            width: parent.width
                            spacing: 4
                            Text {
                                text: "Résolution (densité): " + terrainManager.resolution
                                color: "white"
                                font.pixelSize: 13
                            }
                            Slider {
                                width: parent.width
                                from: 2;
                                to: 1024; stepSize: 1
                                value: terrainManager.resolution
                                onValueChanged: terrainManager.resolution = Math.round(value)
                                onPressedChanged: if (!pressed) terrainManager.generate()
                            }
                        }

                        // Résolution heightmap
                        Column {
                            width: parent.width
                            spacing: 4
                            Text {
                                text: "Résolution heightmap: " + terrainManager.heightmapResolution
                                color: "white"
                                font.pixelSize: 13
                            }
                            ComboBox {
                                width: Math.min(160, parent.width)
                                model: [512, 1024, 2048, 4096]
                                currentIndex: model.indexOf(terrainManager.heightmapResolution)
                                onActivated: terrainManager.heightmapResolution = parseInt(currentText)
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

                // ═══════════════════════════════════════════
                // SECTION: BRUSH
                // ═══════════════════════════════════════════
                Rectangle {
                    width: parent.width
                    height: brushSection.height
                    color: "transparent"

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
                                    color: brushManager.brushIndex === index ? "#bf5934" : "#333"
                                    radius: 6
                                    border.color: brushManager.brushIndex === index ? "#ff8855" : "#555"
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

                        // Brush size
                        Column {
                            width: parent.width
                            spacing: 4
                            Text {
                                text: "Taille: " + brushManager.brushSize.toFixed(1)
                                color: "white"
                                font.pixelSize: 13
                            }
                            Slider {
                                id: brushSizeSlider
                                width: parent.width
                                from: 0.1;
                                to: 10; stepSize: 0.1
                                value: brushManager.brushSize
                                onValueChanged: brushManager.brushSize = value
                            }
                        }

                        // Brush strength
                        Column {
                            width: parent.width
                            spacing: 4
                            Text {
                                text: "Intensité: " + brushManager.brushStrength.toFixed(2)
                                color: "white"
                                font.pixelSize: 13
                            }
                            Slider {
                                id: brushStrengthSlider
                                width: parent.width
                                from: 0.0;
                                to: 5.0; stepSize: 0.05
                                value: brushManager.brushStrength
                                onValueChanged: brushManager.brushStrength = value
                            }
                        }
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {
                id: vScrollBar
                parent: scrollView
                anchors.right: scrollView.right
                anchors.top: scrollView.top
                anchors.bottom: scrollView.bottom
                policy: ScrollBar.AsNeeded
                hoverEnabled: true

                width: 9
                background: Rectangle {
                    color: "transparent"
                }
                contentItem: Rectangle {
                    radius: 6
                    color: vScrollBar.pressed ? "#3f4248" : vScrollBar.hovered ? "#37393e" : "#33353a"
                }
            }
        }
    }
}
