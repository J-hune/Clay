qt_add_library(ClayContent STATIC)
qt6_add_qml_module(ClayContent
    URI "ClayContent"
    VERSION 1.0
    RESOURCE_PREFIX "/qt/qml"
    QML_FILES
        "App.qml"
        "Screen01.ui.qml"
        "components/TopBar.qml"
        "components/SpeedBar.qml"
        "components/DistanceBar.qml"
        "components/DebugOverlay.qml"
        "components/CameraSection.qml"
        "components/TerrainSection.qml"
        "components/BrushSection.qml"
        "components/RightBar.qml"
    RESOURCES
        "images/arrow-rotate-left.svg"
        "images/magnifying-glass-plus.svg"
        "images/person-running.svg"
        "images/layer-plus.svg"
        "fonts/OpenSans-Regular.ttf"
        "fonts/OpenSans-SemiBold.ttf"
        "components/qmldir"
)


