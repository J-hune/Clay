import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

Rectangle {
    id: topBar
    height: 60
    color: "#2e3136"
    border.width: 0

    required property var terrainManager

    property bool terrainPopupVisible: false

    // Alias pour compatibilité
    readonly property var terrainPopup: QtObject
    {
        property bool visible: terrainPopupVisible
    }

    Row {
        id: topRow
        anchors.fill: parent
        anchors.margins: 8
        spacing: 12

        // Bouton Terrain stylisé
        Rectangle {
            id: terrainButton
            width: 100
            height: 44
            radius: 22
            anchors.verticalCenter: parent.verticalCenter
            color: terrainMouseArea.pressed ? "#4a4d52" : (terrainMouseArea.containsMouse ? "#4c566a" : "#3a3d42")
            border.width: 0

            Behavior on color {
                ColorAnimation {
                    duration: 100
                }
            }

            Row {
                anchors.centerIn: parent
                spacing: 8

                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    sourceSize.width: 18
                    sourceSize.height: 18
                    source: "../images/layer-plus.svg"
                    fillMode: Image.PreserveAspectFit
                    opacity: terrainMouseArea.containsMouse ? 1.0 : 0.7
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Terrain"
                    color: "#e5e9f0"
                    font.pixelSize: 14
                    font.weight: Font.Medium
                }
            }

            MouseArea {
                id: terrainMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: terrainPopupVisible = !terrainPopupVisible
            }
        }
    }

    // Popup moderne pour le terrain
    Loader {
        id: popupLoader
        active: terrainPopupVisible

        sourceComponent: Rectangle {
            id: terrainPopupContent
            parent: Overlay.overlay || topBar.Window.contentItem || topBar.parent
            width: 380
            height: contentColumn.implicitHeight + 40
            color: "#272a2f"
            radius: 10
            border.width: 1
            border.color: "#3a3d42"
            x: {
                var buttonGlobal = terrainButton.mapToItem(null, 0, 0)
                return buttonGlobal ? buttonGlobal.x : 8
            }
            y: {
                var buttonGlobal = terrainButton.mapToItem(null, 0, 0)
                return buttonGlobal ? buttonGlobal.y + terrainButton.height + 8 : topBar.height + 8
            }
            z: 10000
            visible: terrainPopupVisible

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
                id: triangleIndicator
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

            Column {
                id: contentColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 20
                spacing: 20

                // Header
                Text {
                    text: "Configuration du terrain"
                    color: "#e5e9f0"
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                }

                // Séparateur
                Rectangle {
                    width: parent.width
                    height: 1
                    color: "#3a3d42"
                }

                // Mode de génération
                Column {
                    width: parent.width
                    spacing: 10

                    Text {
                        text: "Mode de génération"
                        color: "#d8dee9"
                        font.pixelSize: 13
                        font.weight: Font.Medium
                    }

                    Row {
                        spacing: 8
                        width: parent.width

                        Repeater {
                            model: ["Plat", "Heightmap", "Bruit"]

                            Rectangle {
                                width: (parent.width - 16) / 3
                                height: 36
                                radius: 6
                                color: {
                                    var isSelected = (index === terrainManager.mode)
                                    var mouseArea = modeMouseAreas.itemAt(index)
                                    if (!mouseArea) return isSelected ? "#3a3d42" : "transparent"
                                    if (mouseArea.pressed) return "#3a3d42"
                                    if (isSelected) return "#2e3440"
                                    if (mouseArea.containsMouse) return "#272a2f"
                                    return "transparent"
                                }
                                border.width: 1
                                border.color: (index === terrainManager.mode) ? "#4c566a" : "#3a3d42"
                                opacity: index === 2 ? 0.5 : 1.0 // Mode Bruit pas encore implémenté

                                Behavior on color {
                                    ColorAnimation {
                                        duration: 100
                                    }
                                }

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData + (index === 2 ? " (bientôt)" : "")
                                    color: (index === terrainManager.mode) ? "#e5e9f0" : "#a0a0a0"
                                    font.pixelSize: 13
                                    font.weight: (index === terrainManager.mode) ? Font.Medium : Font.Normal
                                }

                                MouseArea {
                                    id: modeMouseArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: index === 2 ? Qt.ForbiddenCursor : Qt.PointingHandCursor
                                    enabled: index !== 2 // Désactiver le mode Bruit pour l'instant
                                    onClicked: {
                                        if (index !== 2) {
                                            terrainManager.mode = index
                                        }
                                    }

                                    Component.onCompleted: {
                                        modeMouseAreas.model = modeMouseAreas.model || []
                                    }
                                }

                                Component.onCompleted: {
                                    if (!modeMouseAreas.itemAt) {
                                        modeMouseAreas.model = []
                                    }
                                }
                            }
                        }

                        Repeater {
                            id: modeMouseAreas
                            model: 0
                        }
                    }
                }

                // Résolution du terrain
                Column {
                    width: parent.width
                    spacing: 10

                    Text {
                        text: "Paramètres du mesh"
                        color: "#d8dee9"
                        font.pixelSize: 13
                        font.weight: Font.Medium
                    }

                    CustomSlider {
                        width: parent.width
                        from: 2
                        to: 1024
                        stepSize: 1
                        value: terrainManager.resolution
                        label: "Résolution (densité)"
                        decimals: 0
                        primaryColor: "#5e81ac"
                        textColor: "#d8dee9"
                        onValueChanged: terrainManager.resolution = Math.round(value)
                    }

                    CustomSlider {
                        width: parent.width
                        from: 0
                        to: 200
                        stepSize: 1
                        value: terrainManager.heightScale
                        label: "Échelle hauteur"
                        decimals: 0
                        primaryColor: "#5e81ac"
                        textColor: "#d8dee9"
                        onValueChanged: terrainManager.heightScale = value
                    }

                    Row {
                        width: parent.width
                        spacing: 8

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "Résolution heightmap:"
                            color: "#a0a0a0"
                            font.pixelSize: 12
                        }

                        ComboBox {
                            id: heightmapResCombo
                            width: 120
                            model: [512, 1024, 2048, 4096]
                            currentIndex: model.indexOf(terrainManager.heightmapResolution)
                            onActivated: terrainManager.heightmapResolution = parseInt(currentText)

                            background: Rectangle {
                                color: heightmapResCombo.pressed ? "#3a3d42" : "#212429"
                                radius: 6
                                border.width: 1
                                border.color: "#3a3d42"
                            }

                            contentItem: Text {
                                text: heightmapResCombo.displayText
                                color: "#e5e9f0"
                                font.pixelSize: 12
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: 10
                            }

                            popup: Popup {
                                y: heightmapResCombo.height
                                width: heightmapResCombo.width
                                implicitHeight: contentItem.implicitHeight
                                padding: 1
                                z: 20000

                                contentItem: ListView {
                                    clip: true
                                    implicitHeight: contentHeight
                                    model: heightmapResCombo.popup.visible ? heightmapResCombo.delegateModel : null
                                    currentIndex: heightmapResCombo.highlightedIndex

                                    ScrollIndicator.vertical: ScrollIndicator {
                                    }
                                }

                                background: Rectangle {
                                    color: "#272a2f"
                                    border.color: "#4c566a"
                                    border.width: 1
                                    radius: 6
                                }
                            }

                            delegate: ItemDelegate {
                                width: heightmapResCombo.width
                                contentItem: Text {
                                    text: modelData
                                    color: "#e5e9f0"
                                    font.pixelSize: 12
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    color: highlighted ? "#4c566a" : (hovered ? "#3a3d42" : "transparent")
                                    radius: 4
                                }
                                highlighted: heightmapResCombo.highlightedIndex === index
                                hoverEnabled: true
                            }
                        }
                    }
                }

                // Paramètres Heightmap (visible uniquement en mode Heightmap)
                Column {
                    width: parent.width
                    spacing: 10
                    visible: terrainManager.mode === 1

                    Text {
                        text: "Fichier heightmap"
                        color: "#d8dee9"
                        font.pixelSize: 13
                        font.weight: Font.Medium
                    }

                    Rectangle {
                        width: parent.width
                        height: 40
                        radius: 6
                        color: imageMouseArea.pressed ? "#3a3d42" : (imageMouseArea.containsMouse ? "#272a2f" : "#212429")
                        border.width: 1
                        border.color: "#3a3d42"

                        Row {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 10

                            Image {
                                anchors.verticalCenter: parent.verticalCenter
                                width: 16
                                height: 16
                                source: "../images/layer-plus.svg"
                                fillMode: Image.PreserveAspectFit
                                opacity: 0.7
                            }

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: terrainManager.heightmapSource.toString().length > 0
                                    ? terrainManager.heightmapSource.toString().split('/').pop()
                                    : "Choisir une image..."
                                color: terrainManager.heightmapSource.toString().length > 0 ? "#e5e9f0" : "#a0a0a0"
                                font.pixelSize: 12
                                elide: Text.ElideRight
                                width: parent.width - 50
                            }
                        }

                        MouseArea {
                            id: imageMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: fileDialog.open()
                        }
                    }

                    FileDialog {
                        id: fileDialog
                        title: "Choisir une image heightmap"
                        onAccepted: {
                            terrainManager.heightmapSource = fileDialog.selectedFile
                        }
                    }
                }

                // Séparateur
                Rectangle {
                    width: parent.width
                    height: 1
                    color: "#3a3d42"
                }

                // Actions
                Row {
                    width: parent.width
                    spacing: 10

                    Rectangle {
                        width: (parent.width - 10) / 2
                        height: 40
                        radius: 6
                        color: generateMouseArea.pressed ? "#5e81ac" : (generateMouseArea.containsMouse ? "#4c72a0" : "#4c566a")
                        border.width: 0

                        Behavior on color {
                            ColorAnimation {
                                duration: 100
                            }
                        }

                        Text {
                            anchors.centerIn: parent
                            text: terrainManager.ready ? "Régénérer" : "Générer"
                            color: "#eceff4"
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                        }

                        MouseArea {
                            id: generateMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: terrainManager.generate()
                        }
                    }

                    Rectangle {
                        width: (parent.width - 10) / 2
                        height: 40
                        radius: 6
                        color: "transparent"
                        border.width: 1
                        border.color: "#4c566a"

                        Column {
                            anchors.centerIn: parent
                            spacing: 2

                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: terrainManager.ready ? "Prêt" : "Non généré"
                                color: terrainManager.ready ? "#a3be8c" : "#a0a0a0"
                                font.pixelSize: 11
                                font.weight: Font.Medium
                            }

                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: terrainManager.ready ? (terrainManager.triangleCount + " triangles") : ""
                                color: "#787878"
                                font.pixelSize: 10
                                visible: terrainManager.ready
                            }
                        }
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
                terrainPopupVisible = false
            }

            focus: visible
            onVisibleChanged: {
                if (visible) forceActiveFocus()
            }
        }
    }

    // Overlay pour détecter les clics en dehors
    Loader {
        id: overlayLoader
        active: terrainPopupVisible
        sourceComponent: MouseArea {
            parent: Overlay.overlay || topBar.Window.contentItem || topBar.parent
            anchors.fill: parent
            z: 9999

            onPressed: (mouse) => {
                // Vérifier si le clic est en dehors du bouton terrain
                var buttonPos = terrainButton.mapToItem(parent, 0, 0)
                var clickInButton = mouse.x >= buttonPos.x && mouse.x <= buttonPos.x + terrainButton.width &&
                    mouse.y >= buttonPos.y && mouse.y <= buttonPos.y + terrainButton.height

                // Si le clic n'est pas dans le bouton, fermer le popup
                // Le popup lui-même bloque ses propres clics avec son MouseArea
                if (!clickInButton) {
                    terrainPopupVisible = false
                }
                mouse.accepted = false
            }
        }
    }
}
