/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick
import QtQuick.Controls
import Clay

// Pour ouvrir le projet sur Qt Design Studio, il faut commenter l'import suivant
// (oui c'est un peu con, mais bon...)
import MyGL 1.0

Rectangle {
    id: rectangle
    anchors.fill: parent
    implicitWidth: Constants.width
    implicitHeight: Constants.height

    color: Constants.backgroundColor

    Rectangle {
        id: rightPanel
        width: 420
        color: "#222222"
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
        property int initialGridResolution: 150

        Column {
            id: controlColumn
            anchors.fill: parent
            anchors.margins: 12
            spacing: 10

            Text {
                text: "Statut Rendu"
                font.bold: true
                font.pixelSize: 16
                color: "white"
            }
            Rectangle {
                height: 1
                width: parent.width
                color: "#444"
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
                        onClicked: glView.mouseSensitivity = rightPanel.initialMouseSensitivity
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
                        onClicked: glView.gridResolution = rightPanel.initialGridResolution
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
                    onToggled: { glView.drawGrid = checked; glView.update(); }
                }
                Switch {
                    id: axesSwitch
                    text: "Axes"
                    checked: glView.drawAxes
                    onToggled: { glView.drawAxes = checked; glView.update(); }
                }
            }
            Rectangle {
                height: 1
                width: parent.width
                color: "#444"
            }
        }
    }

    Rectangle {
        id: leftPanel
        border.width: 0
        anchors.left: rightPanel.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.leftMargin: 0
        anchors.rightMargin: 0
        anchors.topMargin: 0
        anchors.bottomMargin: 0

        MyGLItem {
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
            anchors.topMargin: 96
            anchors.bottomMargin: 96
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
                color: "#39c5ff"
                opacity: 0.85
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
            anchors.topMargin: 96
            anchors.bottomMargin: 96
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
                color: "#ffb347"
                opacity: 0.85
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
