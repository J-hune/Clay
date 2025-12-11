import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle  {
    id: erosionPopupContent

    required property var erosionController
    required property var erosionButton
    required property var topBar
    required property var glViewport
    required property bool erosionPopupVisible

    parent: Overlay.overlay || topBar.Window.contentItem || topBar.parent
    width: 900
    height: Math.min(2800, erosionControlContent.implicitHeight + 40)
    color: "#272a2f"
    radius: 10
    border.width: 1
    border.color: "#3a3d42"
    x: {
        var buttonGlobal = erosionButton.mapToItem(null, 0, 0)
        return buttonGlobal ? buttonGlobal.x : 8
    }
    y: {
        var buttonGlobal = erosionButton.mapToItem(null, 0, 0)
        return buttonGlobal ? buttonGlobal.y + erosionButton.height + 8 : topBar.height + 8
    }
    z: 10000
    visible: erosionPopupVisible

    // Timer pour l'érosion continue
    Timer {
        id: erosionTimer
        interval: 10 // 0.1 secondes
        repeat: true
        running: erosionController ? erosionController.isErosionRunning : false
        onTriggered: {
            if (erosionController) {
                erosionController.applyErosion()
            }
        }
    }

    // Ombre portée
    layer.enabled: true
    layer.effect: ShaderEffect {
        property color shadowColor: "#cc000000"
    }

    // MouseArea pour bloquer les clics et empêcher la fermeture du popup
    MouseArea {
        anchors.fill: parent
        onPressed: (mouse) => {
            mouse.accepted = true
        }
        onReleased: (mouse) => {
            mouse.accepted = true
        }
        onClicked: (mouse) => {
            mouse.accepted = true
        }
    }

    // Triangle indicateur
    Canvas {
        id: erosionTriangleIndicator
        width: 16
        height: 8
        anchors.bottom: parent.top
        anchors.left: parent.left
        anchors.leftMargin: 40
        anchors.bottomMargin: -1

        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();
            ctx.fillStyle = "#272a2f";
            ctx.strokeStyle = "#3a3d42";
            ctx.lineWidth = 1;

            ctx.beginPath();
            ctx.moveTo(width / 2, 0);
            ctx.lineTo(width, height);
            ctx.lineTo(0, height);
            ctx.closePath();
            ctx.fill();
            ctx.stroke();
        }
    }

    // Contenu du contrôle d'érosion
    Item {
        id: erosionControlContent
        implicitHeight: contentColumnErosion.implicitHeight + 20
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 20

        Column {
            id : contentColumnErosion
            width: parent.width
            spacing: 12
            RowLayout {
                width: parent.width
                // alignements verticaux via Layout
                Text {
                    Layout.alignment: Qt.AlignVCenter
                    text: "Paramètres d'érosion hydraulique"
                    color: "#e5e9f0"
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                }

                // spacer flexible qui pousse le bouton à droite
                Item { Layout.fillWidth: true }
                // Bouton pour appliquer l'érosion

                Rectangle {
                    width: 160
                    height: 40
                    radius: 6
                    color: {
                        if (erosionController && erosionController.isErosionRunning) {
                            return generateMouseAreaErosion.pressed ? "#bf616a" : (generateMouseAreaErosion.containsMouse ? "#d08770" : "#d08770")
                        } else {
                            return generateMouseAreaErosion.pressed ? "#5e81ac" : (generateMouseAreaErosion.containsMouse ? "#4c72a0" : "#4c566a")
                        }
                    }
                    border.width: 0

                    Behavior on color {
                        ColorAnimation {
                            duration: 100
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        text: erosionController && erosionController.isErosionRunning ? "Arrêter érosion" : "Appliquer l'érosion"
                        color: "#eceff4"
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                    }

                    MouseArea {
                        id: generateMouseAreaErosion
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (erosionController) {
                                erosionController.toggleErosion()
                                // Si on vient d'arrêter l'érosion -> capture snapshot APRÈS
                                if (!erosionController.isErosionRunning) {
                                    glViewport.captureSnapshot()
                                }
                            }
                        }
                    }
                }
            }


            // Séparateur
            Rectangle {
                width: parent.width
                height: 1
                color: "#3a3d42"
            }

            // Contrôles d'érosion en grille
            Flow {
                width: contentColumnErosion.width
                spacing: 12
                flow: Flow.LeftToRight




                // Particules
                CustomSlider {
                    label: "Particules"
                    from: 1000
                    to: 70000
                    stepSize: 100
                    decimals: 0
                    primaryColor: "#5e81ac"
                    textColor: "#d8dee9"
                    value: erosionController ? erosionController.numParticles : 1000
                    onValueChanged: if (erosionController) erosionController.numParticles = value
                    width: 200
                }

                // Vitesse d'érosion
                CustomSlider {
                    label: "Vitesse d'érosion"
                    from: 0.0
                    to: 0.4
                    stepSize: 0.01
                    decimals: 2
                    primaryColor: "#5e81ac"
                    textColor: "#d8dee9"
                    value: erosionController ? erosionController.erosionSpeed : 0.3
                    onValueChanged: if (erosionController) erosionController.erosionSpeed = value
                    width: 200
                }

                // Capacité sédiment
                CustomSlider {
                    label: "Capacité sédiment"
                    from: 0.0
                    to: 32.0
                    stepSize: 0.1
                    decimals: 1
                    primaryColor: "#5e81ac"
                    textColor: "#d8dee9"
                    value: erosionController ? erosionController.sedimentCapacity : 8.0
                    onValueChanged: if (erosionController) erosionController.sedimentCapacity = value
                    width: 200
                }

                // Dépôt
                CustomSlider {
                    label: "Dépôt"
                    from: 0.0
                    to: 1.0
                    stepSize: 0.01
                    decimals: 2
                    primaryColor: "#5e81ac"
                    textColor: "#d8dee9"
                    value: erosionController ? erosionController.depositionPercentage : 0.1
                    onValueChanged: if (erosionController) erosionController.depositionPercentage = value
                    width: 200
                }

                // Inertie
                CustomSlider {
                    label: "Inertie"
                    from: 0.1
                    to: 1.0
                    stepSize: 0.01
                    decimals: 2
                    primaryColor: "#5e81ac"
                    textColor: "#d8dee9"
                    value: erosionController ? erosionController.inertia : 0.1
                    onValueChanged: if (erosionController) erosionController.inertia = value
                    width: 200
                }

                // Évaporation
                CustomSlider {
                    label: "Évaporation"
                    from: 0.01
                    to: 0.5
                    stepSize: 0.001
                    decimals: 3
                    primaryColor: "#5e81ac"
                    textColor: "#d8dee9"
                    value: erosionController ? erosionController.evaporationSpeed : 0.01
                    onValueChanged: if (erosionController) erosionController.evaporationSpeed = value
                    width: 200
                }

                // Pente minimale
                CustomSlider {
                    label: "Pente minimale"
                    from: 0.0001
                    to: 0.05
                    stepSize: 0.0001
                    decimals: 4
                    primaryColor: "#5e81ac"
                    textColor: "#d8dee9"
                    value: erosionController ? erosionController.minSlope : 0.001
                    onValueChanged: if (erosionController) erosionController.minSlope = value
                    width: 200
                }

                // Rayon d'érosion
                CustomSlider {
                    label: "Rayon d'érosion"
                    from: 0
                    to: 6
                    stepSize: 1
                    decimals: 0
                    primaryColor: "#5e81ac"
                    textColor: "#d8dee9"
                    value: erosionController ? erosionController.erosionRadius : 1
                    onValueChanged: if (erosionController) erosionController.erosionRadius = value
                    width: 200
                }

                // Durée de vie max
                /*CustomSlider {
                    label: "Durée de vie max"
                    from: 1
                    to: 200
                    stepSize: 1
                    decimals: 0
                    primaryColor: "#5e81ac"
                    textColor: "#d8dee9"
                    value: erosionController ? erosionController.maxLifetime : 30
                    onValueChanged: if (erosionController) erosionController.maxLifetime = value
                    width: 200
                }*/

                // Gravité
                /*CustomSlider {
                    label: "Gravité"
                    from: 0.0
                    to: 10.0
                    stepSize: 0.1
                    decimals: 1
                    primaryColor: "#5e81ac"
                    textColor: "#d8dee9"
                    value: erosionController ? erosionController.gravity : 4.0
                    onValueChanged: if (erosionController) erosionController.gravity = value
                    width: 200
                }*/
            }
        }
    }

    // Animation d'apparition
    scale: visible ? 1.0 : 0.95
    opacity: visible ? 1.0 : 0.0

    Behavior on scale {
        NumberAnimation {
            duration: 150; easing.type: Easing.OutCubic
        }
    }
    Behavior on opacity {
        NumberAnimation {
            duration: 150
        }
    }

    // Gestion de la touche Escape
    Keys.onEscapePressed: {
        erosionPopupVisible = false
        if (erosionController && erosionController.isErosionRunning) {
            glViewport.captureSnapshot() // Snapshot APRÈS l'érosion
            erosionController.toggleErosion();
        }
    }

    focus: visible
    onVisibleChanged: {
        if (visible) {
            forceActiveFocus()
        } else if (erosionController && erosionController.isErosionRunni ng) {
            glViewport.captureSnapshot() // Snapshot APRÈS l'érosion
        }
    }
}