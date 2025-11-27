import QtQuick
import QtQuick.Controls

FocusScope {
    id: root

    property real from: 0
    property real to: 100
    property real value: 0
    property real stepSize: 1
    property string label: ""
    property int decimals: 1
    property color backgroundColor: "#2e3136"
    property color fillColor: "#bf5934"
    property color textColor: "white"

    implicitHeight: 28

    Rectangle {
        id: background
        anchors.fill: parent
        color: root.backgroundColor
        radius: 6
        border.width: 1
        border.color: dragArea.dragging ? "#bf5934" : "#52555b"
        clip: true

        // Rectangle de remplissage avec masque arrondi
        Rectangle {
            id: fillRect
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.margins: 1
            width: Math.max(0, (parent.width - 2) * ((root.value - root.from) / (root.to - root.from)))
            color: root.fillColor
            opacity: 0.4
            radius: background.radius - 1
        }

        // Texte avec label et valeur
        Text {
            id: valueText
            anchors.centerIn: parent
            text: root.label + (root.label ? ": " : "") + root.value.toFixed(root.decimals)
            color: root.textColor
            font.pixelSize: 13
            z: 1
            visible: !textInput.visible
        }

        // MouseArea pour drag
        MouseArea {
            id: dragArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: textInput.visible ? Qt.ArrowCursor : (containsMouse ? (dragging ? Qt.ClosedHandCursor : Qt.SizeHorCursor) : Qt.ArrowCursor)
            preventStealing: true
            propagateComposedEvents: false
            enabled: !textInput.visible
            acceptedButtons: Qt.LeftButton | Qt.RightButton

            property real startValue: 0
            property real startX: 0
            property real startY: 0
            property bool dragging: false

            onPressed: (mouse) => {
                startValue = root.value
                startX = mouse.x
                startY = mouse.y
                dragging = true
                mouse.accepted = true
            }

            onPositionChanged: (mouse) => {
                if (dragging) {
                    // Ne prendre en compte que le mouvement horizontal
                    var delta = mouse.x - startX
                    var range = root.to - root.from
                    var change = (delta / width) * range
                    var newValue = Math.max(root.from, Math.min(root.to, startValue + change))

                    // Appliquer stepSize
                    if (root.stepSize > 0) {
                        newValue = Math.round(newValue / root.stepSize) * root.stepSize
                    }

                    root.value = newValue
                    mouse.accepted = true
                }
            }

            onReleased: (mouse) => {
                dragging = false
                mouse.accepted = true
            }

            onDoubleClicked: {
                textInput.visible = true
                textInput.text = root.value.toFixed(root.decimals)
                textInput.selectAll()
                textInput.forceActiveFocus()
            }

            onClicked: (mouse) => {
                if (mouse.button === Qt.RightButton) {
                    textInput.visible = true
                    textInput.text = root.value.toFixed(root.decimals)
                    textInput.selectAll()
                    textInput.forceActiveFocus()
                    mouse.accepted = true
                }
            }
        }

        // TextInput pour édition directe
        TextInput {
            id: textInput
            anchors.centerIn: parent
            visible: false
            color: root.textColor
            font.pixelSize: 13
            horizontalAlignment: TextInput.AlignHCenter
            verticalAlignment: TextInput.AlignVCenter
            selectByMouse: true
            focus: visible
            z: 2

            Rectangle {
                anchors.fill: parent
                anchors.margins: -4
                color: "transparent"
                radius: 3
                z: -1
            }

            onAccepted: {
                var newValue = parseFloat(text)
                if (!isNaN(newValue)) {
                    newValue = Math.max(root.from, Math.min(root.to, newValue))
                    if (root.stepSize > 0) {
                        newValue = Math.round(newValue / root.stepSize) * root.stepSize
                    }
                    root.value = newValue
                }
                visible = false
            }

            Keys.onEscapePressed: {
                visible = false
            }
        }
    }

    // Overlay invisible pour détecter les clics en dehors
    Loader {
        id: overlayLoader
        active: textInput.visible
        sourceComponent: Item {
            parent: Overlay.overlay || root.Window.contentItem || root.parent
            anchors.fill: parent
            z: 999

            MouseArea {
                anchors.fill: parent
                onPressed: (mouse) => {
                    // Vérifier si le clic est en dehors du slider
                    var sliderPos = mapToItem(root, mouse.x, mouse.y)
                    if (sliderPos.x < 0 || sliderPos.x > root.width ||
                        sliderPos.y < 0 || sliderPos.y > root.height) {
                        textInput.visible = false
                    }
                    mouse.accepted = false
                }
            }
        }
    }
}
