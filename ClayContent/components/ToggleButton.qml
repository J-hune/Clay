import QtQuick
import QtQuick.Controls

Rectangle {
    id: toggleButton
    
    width: 36
    height: 36
    radius: 6
    
    required property string iconSource
    required property bool active
    
    signal clicked()
    
    color: {
        if (mouseArea.pressed) return "#3a3d42"
        if (active) return "#2e3440"
        if (mouseArea.containsMouse) return "#272a2f"
        return "transparent"
    }

    Behavior on color { 
        ColorAnimation { duration: 100 } 
    }

    // Indicateur d'activation
    Rectangle {
        width: 24
        height: 2
        radius: 1
        color: "#5e81ac"
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 4
        visible: active
        opacity: active ? 1.0 : 0.0
        
        Behavior on opacity { 
            NumberAnimation { duration: 150 } 
        }
    }

    Image {
        anchors.centerIn: parent
        width: 20
        height: 20
        source: toggleButton.iconSource
        fillMode: Image.PreserveAspectFit
        opacity: active ? 1.0 : 0.5

        Behavior on opacity { 
            NumberAnimation { duration: 100 } 
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: toggleButton.clicked()
    }
}
