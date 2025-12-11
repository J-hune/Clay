import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

Rectangle {
    id: terrainPopupContent
    
    required property var terrainManager

    width: 380
    height: contentColumn.implicitHeight + 40
    color: "#272a2f"
    radius: 10
    border.width: 1
    border.color: "#3a3d42"
    z: 10000

    // Ombre portée
    layer.enabled: true
    layer.effect: ShaderEffect {
        property color shadowColor: "#cc000000"
    }

    // MouseArea pour bloquer les clics et empêcher la fermeture du popup
    MouseArea {
        anchors.fill: parent
        onPressed: (mouse) => { mouse.accepted = true }
        onReleased: (mouse) => { mouse.accepted = true }
        onClicked: (mouse) => { mouse.accepted = true }
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
        TerrainModeSelector {
            width: parent.width
            terrainManager: terrainPopupContent.terrainManager
        }

        // Paramètres du mesh
        TerrainMeshSettings {
            width: parent.width
            terrainManager: terrainPopupContent.terrainManager
        }

        // Paramètres Heightmap (visible uniquement en mode Heightmap)
        TerrainHeightmapSettings {
            width: parent.width
            terrainManager: terrainPopupContent.terrainManager
            visible: terrainManager.mode === 1
        }

        // Séparateur
        Rectangle {
            width: parent.width
            height: 1
            color: "#3a3d42"
        }

        // Actions
        TerrainActions {
            width: parent.width
            terrainManager: terrainPopupContent.terrainManager
        }
    }

    // Animation d'apparition
    scale: visible ? 1.0 : 0.95
    opacity: visible ? 1.0 : 0.0

    Behavior on scale {
        NumberAnimation {
            duration: 150
            easing.type: Easing.OutCubic
        }
    }
    
    Behavior on opacity {
        NumberAnimation {
            duration: 150
        }
    }
}
