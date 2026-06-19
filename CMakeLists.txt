cmake_minimum_required(VERSION 3.16)
project(qt_bilibili_live VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

if(NOT DEFINED CMAKE_PREFIX_PATH)
    list(APPEND CMAKE_PREFIX_PATH "D:/Qt/6.11.1/mingw_64")
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Multimedia MultimediaWidgets)

set(BILI_LIVE_SOURCES
    src/main.cpp
    src/core/app_settings.cpp
    src/core/bilibili_utils.cpp
    src/core/grid_layout.cpp
    src/proxy/local_stream_proxy.cpp
    src/ui/live_stream_tile.cpp
    src/ui/bilibili_live_window.cpp
)

add_executable(qt_bilibili_live WIN32 ${BILI_LIVE_SOURCES})

target_include_directories(qt_bilibili_live PRIVATE src)

target_link_libraries(qt_bilibili_live PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia
    Qt6::MultimediaWidgets
)

if(WIN32 AND MINGW)
    set_target_properties(qt_bilibili_live PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# ---------------------------------------------------------------------------
# Windows 安装与打包（CPack: ZIP / NSIS）
# ---------------------------------------------------------------------------
if(WIN32)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/dist"
        CACHE PATH "Install directory for packaging" FORCE)

    install(TARGETS qt_bilibili_live
        RUNTIME DESTINATION bin
    )

    # exe 与 Qt 运行时 DLL 均安装到 bin/，避免安装后缺库
    file(WRITE "${CMAKE_BINARY_DIR}/deploy_qt_bilibili_live.cmake"
"set(QT_DEPLOY_BIN_DIR \"bin\")
include(\"${CMAKE_BINARY_DIR}/.qt/QtDeploySupport.cmake\")
include(\"\${CMAKE_CURRENT_LIST_DIR}/qt_bilibili_live-plugins.cmake\" OPTIONAL)
qt6_deploy_runtime_dependencies(
    EXECUTABLE \"${CMAKE_BINARY_DIR}/qt_bilibili_live.exe\"
    BIN_DIR \"bin\"
    GENERATE_QT_CONF
    NO_TRANSLATIONS
)
")
    install(SCRIPT "${CMAKE_BINARY_DIR}/deploy_qt_bilibili_live.cmake")

    set(CPACK_PACKAGE_NAME "qt_bilibili_live")
    set(CPACK_PACKAGE_VENDOR "bench")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Bilibili multi-stream live viewer")
    set(CPACK_PACKAGE_FILE_NAME
        "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-win64")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "qt_bilibili_live")
    set(CPACK_MONOLITHIC_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH OFF)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY "bin")

    set(CPACK_GENERATOR "ZIP;NSIS")

    set(CPACK_NSIS_DISPLAY_NAME "Bilibili Live")
    set(CPACK_NSIS_PACKAGE_NAME "Bilibili Live")
    set(CPACK_NSIS_HELP_LINK "https://live.bilibili.com/")
    set(CPACK_PACKAGE_EXECUTABLES "qt_bilibili_live;Bilibili Live")
    set(CPACK_CREATE_DESKTOP_LINKS "qt_bilibili_live")

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/installer/NSIS.InstallOptions.ini"
        "${CMAKE_BINARY_DIR}/NSIS.InstallOptions.ini"
        COPYONLY
    )
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/installer_license.txt")

    include(CPack)
endif()
