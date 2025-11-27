import QtQuick
import QtQuick.Controls

Rectangle {
    id: gridControl
    width: controlRow.width + 16
    height: 48
    color: "#1e2024"
    radius: 8
    border.width: 1
    border.color: "#3a3d42"

    required property var glView
    required property var cameraController

    Row {
        id: controlRow
        anchors.centerIn: parent
        spacing: 6

        // Bouton Grid
        Rectangle {
            id: gridButton
            width: 36
            height: 36
            radius: 6
            color: {
                if (gridMouseArea.pressed) return "#3a3d42"
                if (glView.drawGrid) return "#2e3440"
                if (gridMouseArea.containsMouse) return "#272a2f"
                return "transparent"
            }

            Behavior on color { ColorAnimation { duration: 100 } }

            // Indicateur d'activation
            Rectangle {
                width: 24
                height: 2
                radius: 1
                color: "#5e81ac"
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottomMargin: 4
                visible: glView.drawGrid
                opacity: glView.drawGrid ? 1.0 : 0.0
                Behavior on opacity { NumberAnimation { duration: 150 } }
            }

            Image {
                anchors.centerIn: parent
                width: 20
                height: 20
                source: "../images/grid.svg"
                fillMode: Image.PreserveAspectFit
                opacity: glView.drawGrid ? 1.0 : 0.5

                Behavior on opacity { NumberAnimation { duration: 100 } }
            }

            MouseArea {
                id: gridMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: glView.drawGrid = !glView.drawGrid
            }
        }

        // Bouton Axes
        Rectangle {
            id: axesButton
            width: 36
            height: 36
            radius: 6
            color: {
                if (axesMouseArea.pressed) return "#3a3d42"
                if (glView.drawAxes) return "#2e3440"
                if (axesMouseArea.containsMouse) return "#272a2f"
                return "transparent"
            }

            Behavior on color { ColorAnimation { duration: 100 } }

            // Indicateur d'activation
            Rectangle {
                width: 24
                height: 2
                radius: 1
                color: "#5e81ac"
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottomMargin: 4
                visible: glView.drawAxes
                opacity: glView.drawAxes ? 1.0 : 0.0
                Behavior on opacity { NumberAnimation { duration: 150 } }
            }

            Image {
                anchors.centerIn: parent
                width: 20
                height: 20
                source: "../images/empty_axis.svg"
                fillMode: Image.PreserveAspectFit
                opacity: glView.drawAxes ? 1.0 : 0.5

                Behavior on opacity { NumberAnimation { duration: 100 } }
            }

            MouseArea {
                id: axesMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: glView.drawAxes = !glView.drawAxes
            }
        }

        // Séparateur
        Rectangle {
            width: 1
            height: 24
            color: "#3a3d42"
            anchors.verticalCenter: parent.verticalCenter
        }

        // Bouton Chevron
        Rectangle {
            id: chevronButton
            width: 36
            height: 36
            radius: 6
            color: {
                if (chevronMouseArea.pressed) return "#3a3d42"
                if (chevronMouseArea.containsMouse) return "#272a2f"
                return "transparent"
            }

            Behavior on color { ColorAnimation { duration: 100 } }

            Image {
                anchors.centerIn: parent
                width: 16
                height: 16
                source: "../images/chevron-down.svg"
                fillMode: Image.PreserveAspectFit
                opacity: chevronMouseArea.containsMouse ? 0.9 : 0.6
                rotation: popup.visible ? 180 : 0

                Behavior on rotation { NumberAnimation { duration: 200 } }
                Behavior on opacity { NumberAnimation { duration: 100 } }
            }

            MouseArea {
                id: chevronMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: popup.visible = !popup.visible
            }
        }
    }

    // Popup pour la résolution de la grille
    Rectangle {
        id: popup
        visible: false
        width: 300
        height: popupColumn.implicitHeight + 32
        color: "#272a2f"
        radius: 10
        border.width: 1
        border.color: "#3a3d42"
        anchors.top: parent.bottom
        anchors.topMargin: 12
        anchors.right: parent.right

        // Ombre portée
        layer.enabled: true
        layer.effect: ShaderEffect {
            property color shadowColor: "#cc000000"
        }

        // Petit triangle pointant vers le haut (indicateur)
        Canvas {
            id: triangleIndicator
            width: 16
            height: 8
            anchors.bottom: parent.top
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.bottomMargin: -1

            onPaint: {
                var ctx = getContext("2d");
                ctx.reset();
                ctx.fillStyle = "#272a2f";
                ctx.strokeStyle = "#3a3d42";
                ctx.lineWidth = 1;

                // Triangle
                ctx.beginPath();
                ctx.moveTo(width/2, 0);
                ctx.lineTo(width, height);
                ctx.lineTo(0, height);
                ctx.closePath();
                ctx.fill();
                ctx.stroke();
            }
        }

        Column {
            id: popupColumn
            anchors.fill: parent
            anchors.topMargin: 16
            anchors.bottomMargin: 16
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            spacing: 16

            Text {
                text: "Paramètres d'affichage"
                color: "#e5e9f0"
                font.pixelSize: 15
                font.weight: Font.Medium
            }

            CustomSlider {
                id: gridResolutionSlider
                width: parent.width
                from: 10
                to: 200
                stepSize: 5
                value: glView.gridResolution
                label: "Résolution grille"
                decimals: 0
                primaryColor: "#5e81ac"
                textColor: "#fff"
                onValueChanged: glView.gridResolution = Math.round(value)
            }

            CustomSlider {
                id: mouseSensitivitySlider
                width: parent.width
                from: 0.01
                to: 0.5
                stepSize: 0.01
                value: cameraController.mouseSensitivity
                label: "Sensibilité souris"
                decimals: 2
                primaryColor: "#5e81ac"
                textColor: "#fff"
                onValueChanged: cameraController.mouseSensitivity = value
            }
        }

        // Animation d'apparition
        scale: visible ? 1.0 : 0.95
        opacity: visible ? 1.0 : 0.0

        Behavior on scale {
            NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
        }
        Behavior on opacity {
            NumberAnimation { duration: 150 }
        }

        // Gestion de la touche Escape
        Keys.onEscapePressed: {
            popup.visible = false
        }

        // Focus pour capturer les événements clavier
        focus: visible
        Component.onCompleted: {
            if (visible) forceActiveFocus()
        }
        onVisibleChanged: {
            if (visible) forceActiveFocus()
        }

        // Fermer le popup en cliquant en dehors
        MouseArea {
            anchors.fill: parent
            propagateComposedEvents: true
            onPressed: (mouse) => { mouse.accepted = false }
        }
    }

    // Overlay pour détecter les clics en dehors du popup
    Loader {
        id: overlayLoader
        active: popup.visible
        sourceComponent: Item {
            parent: Overlay.overlay || gridControl.Window.contentItem || gridControl.parent
            anchors.fill: parent
            z: 998

            MouseArea {
                anchors.fill: parent
                onPressed: (mouse) => {
                    // Vérifier si le clic est en dehors du popup et du bouton chevron
                    var popupPos = mapToItem(popup, mouse.x, mouse.y)
                    var chevronPos = mapToItem(chevronButton, mouse.x, mouse.y)

                    var outsidePopup = popupPos.x < 0 || popupPos.x > popup.width ||
                                      popupPos.y < 0 || popupPos.y > popup.height
                    var outsideChevron = chevronPos.x < 0 || chevronPos.x > chevronButton.width ||
                                        chevronPos.y < 0 || chevronPos.y > chevronButton.height

                    if (outsidePopup && outsideChevron) {
                        popup.visible = false
                    }
                    mouse.accepted = false
                }
            }
        }
    }

    // Raccourci clavier Escape au niveau du composant
    Keys.onEscapePressed: {
        if (popup.visible) {
            popup.visible = false
        }
    }
}

