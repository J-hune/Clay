import QtQuick

// Overlay d'information sur la caméra / rendu
Rectangle {
    id: debugOverlay
    z: 10
    width: 240
    color: "#00000088"
    radius: 6
    border.width: 1
    border.color: "#444"

    required property var glView
    required property var cameraController

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

