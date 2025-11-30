/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick
import QtQuick.Controls
import Clay
import "components"

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

    // ========================================
    // Interface utilisateur
    // ========================================

    TopBar {
        id: topBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        terrainManager: terrainManager
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

        // Barre vitesse FPS
        SpeedBar {
            id: speedBar
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.topMargin: 128
            anchors.bottomMargin: 128
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            cameraController: cameraController
            otherBar: distanceBar
        }

        // Barre distance orbit
        DistanceBar {
            id: distanceBar
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.topMargin: 128
            anchors.bottomMargin: 128
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            cameraController: cameraController
            otherBar: speedBar
        }

        // Overlay d'information sur la caméra / rendu
        DebugOverlay {
            id: debugOverlay
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: 8
            anchors.topMargin: 8
            glView: glView
            cameraController: cameraController
        }

        // Contrôles de grille en haut à droite
        GridControl {
            id: gridControl
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.rightMargin: 8
            anchors.topMargin: 8
            glView: glView
            cameraController: cameraController
        }
    }

    RightBar {
        id: rightBar
        anchors.right: parent.right
        anchors.top: topBar.bottom
        anchors.bottom: parent.bottom
        cameraController: cameraController
        brushManager: brushManager
        terrainManager: terrainManager
        glView: glView
    }
}
