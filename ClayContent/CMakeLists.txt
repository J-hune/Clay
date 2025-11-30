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
        "components/GridControl.qml"
        "components/BrushSection.qml"
        "components/BrushOperations.qml"
        "components/BrushContextMenu.qml"
        "components/RightBar.qml"
        "components/CustomSlider.qml"
    RESOURCES
        "images/arrow-rotate-left.svg"
        "images/magnifying-glass-plus.svg"
        "images/person-running.svg"
        "images/layer-plus.svg"
        "images/arrow-down-to-bracket.svg"
        "images/grid.svg"
        "images/empty_axis.svg"
        "images/chevron-down.svg"
        "images/plus.svg"
        "images/circle-xmark.svg"
        "images/folder-magnifying-glass.svg"
        "fonts/OpenSans-Regular.ttf"
        "fonts/OpenSans-SemiBold.ttf"
        "components/qmldir"
)


