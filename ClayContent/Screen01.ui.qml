/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
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

    Rectangle {
        id: leftBar
        width: 420
        color: "#272a2f"
        border.width: 0
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.leftMargin: 0
        anchors.topMargin: 0
        anchors.bottomMargin: 0

        // Valeurs initiales pour les resets
        property real initialYaw: -90
        property real initialPitch: 0
        property real initialMouseSensitivity: 0.15
        property int initialGridResolution: 50

        Column {
            id: controlColumn
            anchors.fill: parent
            anchors.margins: 12
            spacing: 10

            Text {
                text: "Statut Rendu"
                font.pixelSize: 18
                color: "white"
            }
            Rectangle {
                height: 2
                width: parent.width
                color: "#2c2f34"
            }

            // Contrôle sensibilité souris
            Column {
                spacing: 2
                Text {
                    text: "Sensibilité souris: " + glView.mouseSensitivity.toFixed(2)
                    color: "white"
                    font.pixelSize: 14
                }
                Row {
                    spacing: 6
                    Slider {
                        id: sensSlider
                        from: 0; to: 1; stepSize: 0.01
                        value: glView.mouseSensitivity
                        onValueChanged: glView.mouseSensitivity = value
                        width: 160
                    }
                    Button {
                        icon.height: 16; icon.width: 16
                        icon.source: "images/arrow-rotate-left.svg"
                        onClicked: glView.mouseSensitivity = leftBar.initialMouseSensitivity
                    }
                }
            }
            // Contrôle résolution grille
            Column {
                spacing: 2
                Text {
                    text: "Résolution grille: " + glView.gridResolution
                    color: "white"
                    font.pixelSize: 14
                }
                Row {
                    spacing: 8
                    Slider {
                        id: gridSlider
                        from: 1; to: 500; stepSize: 1
                        value: glView.gridResolution
                        onValueChanged: glView.gridResolution = Math.round(value)
                        width: 140
                    }
                    Button {
                        icon.source: "images/arrow-rotate-left.svg"
                        icon.height: 16; icon.width: 16
                        onClicked: glView.gridResolution = leftBar.initialGridResolution
                    }
                }
            }
            Row {
                // Switch grille / axes
                spacing: 24
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
            Rectangle {
                height: 1
                width: parent.width
                color: "#444"
            }

            // Densité terrain & résolution heightmap
            Column {
                spacing: 4
                Text { text: "Résolution terrain (densité): " + glView.terrainResolution; color: "white"; font.pixelSize: 14 }
                Slider {
                    from: 2; to: 1024; stepSize: 1
                    value: glView.terrainResolution
                    width: 260
                    onValueChanged: glView.terrainResolution = Math.round(value);
                    onPressedChanged: if (!pressed) glView.generateTerrain()
                }
                Text { text: "Résolution heightmap: " + glView.heightmapResolution; color: "white"; font.pixelSize: 14 }
                ComboBox { model: [512,1024,2048,4096]; currentIndex: model.indexOf(glView.heightmapResolution); onActivated: glView.heightmapResolution = parseInt(currentText); width: 140 }
            }
        }
    }

    Rectangle {
        id: topBar
        height: 54
        color: "#212429"
        border.width: 0
        anchors.left: leftBar.right
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
                Text { text: "Génération Terrain"; font.pixelSize: 16; color: "white" }
                Rectangle { height: 1; width: parent.width; color: "#333" }
                Row {
                    spacing: 6
                    Text { text: "Mode:"; color: "white" }
                    ComboBox {
                        id: modeCombo
                        model: ["Plat", "Heightmap"]
                        currentIndex: glView.terrainMode
                        onCurrentIndexChanged: glView.terrainMode = currentIndex
                        width: 120
                    }
                }
                Row {
                    spacing: 6
                    Text { text: "HeightScale:"; color: "white" }
                    Slider {
                        id: heightScaleSlider
                        from: 0; to: 200; stepSize: 1
                        value: glView.heightScale
                        onValueChanged: glView.heightScale = value
                        width: 120
                    }
                    Text { text: Math.round(glView.heightScale); color: "#ccc" }
                }
                Row {
                    spacing: 6
                    visible: glView.terrainMode === 1
                    Button { text: "Image..."; onClicked: fileDialog.open() }
                    Text { text: glView.heightmapSource.toString().length > 0 ? glView.heightmapSource.toString().split('/').pop() : "(Aucune)"; color: "#ccc"; elide: Text.ElideRight; width: 110 }
                }
                FileDialog {
                    id: fileDialog
                    title: "Choisir une image heightmap"
                    onAccepted: {
                        glView.heightmapSource = fileDialog.selectedFile
                    }
                }
                Button {
                    text: glView.terrainReady ? "Régénérer" : "Générer"
                    onClicked: glView.generateTerrain()
                }
                Text { text: glView.terrainReady ? ("Triangles: " + ( (glView.terrainResolution-1)*(glView.terrainResolution-1)*2)) : "Terrain non prêt"; color: "#ccc"; font.pixelSize: 12 }
                Button { text: "Fermer"; onClicked: terrainPopup.close() }
            }
            background: Rectangle { radius: 6; color: "#212429"; border.color: "#444"; border.width: 1 }
        }
    }

    Rectangle {
        id: rightBar
        border.width: 0
        anchors.left: leftBar.right
        anchors.right: parent.right
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
            onCameraSpeedChanged: {speedBar.showTemp()}
            onOrbitDistanceChanged: {distanceBar.showTemp()}
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
            property real value: glView.cameraSpeed
            property real minVal: glView.cameraSpeedMin
            property real maxVal: glView.cameraSpeedMax
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
            Timer { id: hideTimer; interval: speedBar.timeoutMs; running: false; repeat: false; onTriggered: speedBar.visible = false }
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
            property real value: glView.orbitDistance
            property real minVal: glView.orbitDistanceMin
            property real maxVal: glView.orbitDistanceMax
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

            Timer { id: hideTimer2; interval: distanceBar.timeoutMs; running: false; repeat: false; onTriggered: distanceBar.visible = false }
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
                    text: "Pos: " + glView.cameraPosition.x.toFixed(1) + ", " + glView.cameraPosition.y.toFixed(1) + ", " + glView.cameraPosition.z.toFixed(1)
                    color: "white"
                    font.pixelSize: 14
                }
                Text {
                    text: "Yaw: " + glView.yaw.toFixed(1) + "  Pitch: " + glView.pitch.toFixed(1)
                    color: "white"
                    font.pixelSize: 14
                }
            }
        }
    }
}
