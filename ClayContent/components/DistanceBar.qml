import QtQuick

// Barre distance orbit (min=plein, max=vide)
Item {
    id: distanceBar
    visible: false
    width: 12

    required property var cameraController
    property var otherBar: null
    property alias hideTimer: hideTimer

    property real value: cameraController.orbitDistance
    property real minVal: cameraController.orbitDistanceMin
    property real maxVal: cameraController.orbitDistanceMax
    property int timeoutMs: 2000

    function ratio() {
        var span = maxVal - minVal;
        if (span <= 0) return 1;
        var r = 1 - (value - minVal) / span;
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
            id: distanceFill
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: parent.height * distanceBar.ratio()
            radius: parent.radius
            color: "#BF5934"
        }
    }

    Image {
        id: distanceImage
        anchors.top: barBackground.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 6
        width: 15
        height: 15
        opacity: 0.8
        source: "../images/magnifying-glass-plus.svg"
        fillMode: Image.PreserveAspectFit
    }

    Timer {
        id: hideTimer
        interval: distanceBar.timeoutMs
        running: false
        repeat: false
        onTriggered: distanceBar.visible = false
    }

    onValueChanged: distanceFill.height = barBackground.height * ratio()
}

