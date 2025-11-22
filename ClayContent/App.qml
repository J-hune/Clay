import QtQuick
import Clay

Window {
    // On récupère la taille de l'écran et on ouvre l'application à un scale 0.6 x Taille écran
    property real screenW: Screen.width
    property real screenH: Screen.height

    property real scaleFactor: 0.6

    width: Math.round(screenW * scaleFactor)
    height: Math.round(screenH * scaleFactor)

    visible: true
    title: "Clay"

    Screen01 {
        id: mainScreen
    }
}