import QtQuick
import QtQuick.Controls

Rectangle {
    id: rightBar
    width: 400
    color: "#272a2f"
    border.width: 0

    required property var cameraController
    required property var brushManager
    required property var terrainManager
    required property var glView

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
                    const minW = 324;
                    const maxW = 800;
                    
                    // S'assurer que l'openGLArea reste au minimum 320px de large
                    const parentWidth = rightBar.parent ? rightBar.parent.width : 1024;
                    const maxAllowedWidth = parentWidth - 320;
                    const effectiveMaxW = Math.min(maxW, maxAllowedWidth);
                    
                    rightBar.width = Math.max(minW, Math.min(effectiveMaxW, startW - delta));
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

    Flickable {
        id: scrollView
        anchors.fill: parent
        anchors.leftMargin: 12
        contentHeight: controlColumn.height
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        Column {
            id: controlColumn
            anchors.left: parent.left
            anchors.right: parent.right
            topPadding: 10
            bottomPadding: 10
            anchors.rightMargin: vScrollBar.width + 6
            spacing: 10


            // ═══════════════════════════════════════════
            // SECTION: BRUSH
            // ═══════════════════════════════════════════
            BrushSection {
                width: parent.width
                brushManager: rightBar.brushManager
                glView: rightBar.glView
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

