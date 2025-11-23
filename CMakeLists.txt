cmake_minimum_required(VERSION 3.21.1)

option(LINK_INSIGHT "Link Qt Insight Tracker library" ON)
option(BUILD_QDS_COMPONENTS "Build design studio components" ON)

project(ClayApp LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml)
set(QML_IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
    CACHE STRING "Import paths for Qt Creator's code model"
    FORCE
)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets Qml Quick QuickTimeline ShaderTools)
qt_standard_project_setup()


# ------------------------------------------------------
# Récupération automatique des sources dans App
# ------------------------------------------------------
file(GLOB_RECURSE PROJECT_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/App/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/App/*.h"
)

qt_add_executable(${CMAKE_PROJECT_NAME} ${PROJECT_SOURCES})

# Copie le dossier App/brushes dans le dossier où se situe l'exécutable après build
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/App/brushes"
        "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/brushes"
)

qt_add_resources(${CMAKE_PROJECT_NAME} "configuration"
    PREFIX "/"
    FILES
        qtquickcontrols2.conf)

include(qds)

if (BUILD_QDS_COMPONENTS)
    include(qmlcomponents OPTIONAL)
endif()

if (LINK_INSIGHT)
    include(insight OPTIONAL)
endif ()

include(GNUInstallDirs)
install(TARGETS ${CMAKE_PROJECT_NAME}
  BUNDLE DESTINATION .
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/App/brushes DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME})
