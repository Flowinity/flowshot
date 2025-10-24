cmake_minimum_required(VERSION 4.1)
project(Flowshot2)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets DBus Network)

if(UNIX AND NOT APPLE)
    add_compile_definitions(USE_WAYLAND_CLIPBOARD=1)
    set(USE_WAYLAND_CLIPBOARD TRUE)
else()
    add_compile_definitions(USE_WAYLAND_CLIPBOARD=0)
endif()


add_executable(flowshot main.cpp main.h
        data/flowshot2.rc
        utils/filenamehandler.cpp
        utils/filenamehandler.h
        utils/flowinity/EndpointsJSON.cpp
        utils/flowinity/EndpointsJSON.h
        utils/widgets/imagelabel.cpp
        utils/widgets/imagelabel.h
        utils/abstractlogger.cpp
        utils/abstractlogger.h
        utils/strfparse.cpp
        utils/strfparse.h
        utils/desktopinfo.cpp
        utils/desktopinfo.h
        utils/clipboard.cpp
        utils/clipboard.h
        ipc/dbus/flowshotdbusadapter.cpp
        ipc/dbus/flowshotdbusadapter.h
        uploader/imguploadermanager.cpp
        uploader/imguploadermanager.h
        uploader/imguploaderbase.cpp
        uploader/imguploaderbase.h
        uploader/privateuploader/privateuploader.cpp
        uploader/privateuploader/privateuploader.h
        uploader/privateuploader/privateuploaderupload.cpp
        uploader/privateuploader/privateuploaderupload.h
        app/ScreenshotManager.cpp
        app/ScreenshotManager.h
        utils/rng.cpp
        utils/rng.h
        app/Application.cpp
        app/Application.h
        utils/ConfigHandler.cpp
        utils/ConfigHandler.h
        utils/ValueHandler.cpp
        utils/ValueHandler.h
        utils/clipboard.cpp
        utils/clipboard.h
        app/pages/settings/generalconf2.cpp
        app/pages/settings/generalconf2.h)

target_link_libraries(flowshot PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::DBus
        Qt6::Network
)

if(USE_WAYLAND_CLIPBOARD)
    find_package(KF6GuiAddons)
    target_link_libraries(flowshot PRIVATE   KF6::GuiAddons)
endif()


if (UNIX)
    # Install desktop files, completion and dbus files
    configure_file(${CMAKE_SOURCE_DIR}/data/desktopEntry/package/com.flowinity.flowshot2.desktop
            ${CMAKE_CURRENT_BINARY_DIR}/share/applications/com.flowinity.flowshot2.desktop @ONLY)

    configure_file(${CMAKE_SOURCE_DIR}/data/appdata/com.flowinity.flowshot2.metainfo.xml
            ${CMAKE_CURRENT_BINARY_DIR}/share/metainfo/com.flowinity.flowshot2.metainfo.xml COPYONLY)

    configure_file(${CMAKE_SOURCE_DIR}/data/dbus/com.flowinity.flowshot2.xml
            ${CMAKE_CURRENT_BINARY_DIR}/share/dbus-1/interfaces/com.flowinity.flowshot2.xml COPYONLY)

    configure_file(${CMAKE_SOURCE_DIR}/data/dbus/com.flowinity.flowshot2.service.in
            ${CMAKE_CURRENT_BINARY_DIR}/share/dbus-1/services/com.flowinity.flowshot2.service)

    # Install Icons
    configure_file(${CMAKE_SOURCE_DIR}/data/img/hicolor/48x48/apps/com.flowinity.flowshot2.png
            ${CMAKE_CURRENT_BINARY_DIR}/share/icons/hicolor/48x48/apps/com.flowinity.flowshot2.png COPYONLY)

    configure_file(${CMAKE_SOURCE_DIR}/data/img/hicolor/128x128/apps/com.flowinity.flowshot2.png
            ${CMAKE_CURRENT_BINARY_DIR}/share/icons/hicolor/128x128/apps/com.flowinity.flowshot2.png COPYONLY)

    configure_file(${CMAKE_SOURCE_DIR}/data/img/hicolor/scalable/apps/com.flowinity.flowshot2.svg
            ${CMAKE_CURRENT_BINARY_DIR}/share/icons/hicolor/scalable/apps/com.flowinity.flowshot2.svg COPYONLY)

    # Install icon with both names
    configure_file(${CMAKE_SOURCE_DIR}/data/img/hicolor/48x48/apps/flowshot2.png
            ${CMAKE_CURRENT_BINARY_DIR}/share/icons/hicolor/48x48/apps/flowshot2.png COPYONLY)

    configure_file(${CMAKE_SOURCE_DIR}/data/img/hicolor/128x128/apps/flowshot2.png
            ${CMAKE_CURRENT_BINARY_DIR}/share/icons/hicolor/128x128/apps/flowshot2.png COPYONLY)

    configure_file(${CMAKE_SOURCE_DIR}/data/img/hicolor/scalable/apps/flowshot2.svg
            ${CMAKE_CURRENT_BINARY_DIR}/share/icons/hicolor/scalable/apps/flowshot2.svg COPYONLY)

    # Install assets
    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/share/ DESTINATION ${CMAKE_INSTALL_DATAROOTDIR})
endif ()

qt_add_resources(RESOURCES resources.qrc)
target_sources(flowshot PRIVATE ${RESOURCES})