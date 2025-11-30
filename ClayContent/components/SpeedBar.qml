import QtQuick

// Barre vitesse FPS (min=plein, max=vide)
Item {
    id: speedBar
    visible: false
    width: 12

    required property var cameraController
    property var otherBar: null
    property alias hideTimer: hideTimer

    property real value: cameraController.speed
    property real minVal: cameraController.speedMin
    property real maxVal: cameraController.speedMax
    property int timeoutMs: 2000

    function ratio() {
        // min -> 1, max -> 0
        var span = maxVal - minVal;
        if (span <= 0) return 1;
        var r = (value - minVal) / span;
        return Math.max(0, Math.min(1, r));
    }

    function showTemp() {
        if (otherBar && otherBar.hideTimer) {
            otherBar.visible = false;
            otherBar.hideTimer.stop();
        }
        visible = true;
        hideTimer.restart();
    }

    Rectangle {
        id: barBackground
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        radius: 4
        color: "#222"
        border.width: 1
        border.color: "#444"
        clip: true

        Rectangle {
            id: speedFill
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: parent.height * speedBar.ratio()
            radius: parent.radius
            color: "#866ab6"
        }
    }

    Image {
        id: speedImage
        anchors.top: barBackground.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 6
        width: 15
        height: 15
        opacity: 0.8
        source: "../images/person-running.svg"
        fillMode: Image.PreserveAspectFit
    }

    Timer {
        id: hideTimer
        interval: speedBar.timeoutMs
        running: false
        repeat: false
        onTriggered: speedBar.visible = false
    }

    // Mise à jour continue
    onValueChanged: speedFill.height = barBackground.height * ratio()
}

