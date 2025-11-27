import QtQuick

// Barre distance orbit (min=plein, max=vide)
Rectangle {
    id: distanceBar
    visible: false
    width: 12
    radius: 4
    color: "#222"
    border.width: 1
    border.color: "#444"

    required property var cameraController

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
        hideTimer.restart();
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

    onValueChanged: distanceFill.height = height * ratio()
}

