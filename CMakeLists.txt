cmake_minimum_required(VERSION 3.16)

# Set version from environment variable or use git commit hash
if(DEFINED ENV{VERSION_TAG})
    set(APP_VERSION_STRING $ENV{VERSION_TAG})
elseif(DEFINED VERSION_TAG)
    set(APP_VERSION_STRING ${VERSION_TAG})
else()
    # Get short commit hash from git
    execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(GIT_COMMIT_HASH)
        set(APP_VERSION_STRING "${GIT_COMMIT_HASH}")
    else()
        set(APP_VERSION_STRING "unknown")
    endif()
endif()

project(discord-drawing-rpc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "Building version: ${APP_VERSION_STRING}")

# Set build date
if(DEFINED ENV{BUILD_DATE})
    set(BUILD_DATE $ENV{BUILD_DATE})
else()
    string(TIMESTAMP BUILD_DATE "%Y-%m-%d")
endif()

# Flatpak build option
option(FLATPAK_BUILD "Build for Flatpak" OFF)
if(FLATPAK_BUILD)
    set(FLATPAK_ID "com.TheGameratorT.DiscordDrawingRPC")
endif()

# Configure version header
configure_file(
    "${CMAKE_SOURCE_DIR}/src/common/Version.h.in"
    "${CMAKE_BINARY_DIR}/generated/Version.h"
    @ONLY
)
include_directories(${CMAKE_BINARY_DIR}/generated)

# Configure desktop file with appropriate icon name
if(FLATPAK_BUILD)
    set(DESKTOP_ICON_NAME "${FLATPAK_ID}")
else()
    set(DESKTOP_ICON_NAME "discord-drawing-rpc")
endif()
configure_file(
    "${CMAKE_SOURCE_DIR}/resources/discord-drawing-rpc.desktop.in"
    "${CMAKE_BINARY_DIR}/generated/discord-drawing-rpc.desktop"
    @ONLY
)

# Configure Flatpak metainfo file
if(FLATPAK_BUILD)
    configure_file(
        "${CMAKE_SOURCE_DIR}/flatpak/com.TheGameratorT.DiscordDrawingRPC.metainfo.xml.in"
        "${CMAKE_BINARY_DIR}/flatpak/com.TheGameratorT.DiscordDrawingRPC.metainfo.xml"
        @ONLY
    )
endif()

# Configure installer package.xml (Windows only)
if(WIN32)
    configure_file(
        "${CMAKE_SOURCE_DIR}/installer/packages/com.TheGameratorT.app/meta/package.xml.in"
        "${CMAKE_BINARY_DIR}/installer/packages/com.TheGameratorT.app/meta/package.xml"
        @ONLY
    )
    configure_file(
        "${CMAKE_SOURCE_DIR}/installer/config/config.xml.in"
        "${CMAKE_BINARY_DIR}/installer/config/config.xml"
        @ONLY
    )
endif()

# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

# Auto-generate MOC, UIC, and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)

add_library(discord_common SHARED
    src/common/Common.cpp
    src/common/Config.cpp
    src/common/PlatformUtils.cpp
    src/common/DaemonIPC.cpp
)

target_link_libraries(discord_common
    PUBLIC Qt6::Core Qt6::Widgets Qt6::Network
)

# Discord RPC Daemon
add_executable(discord-drawing-rpc-daemon
    src/daemon/main.cpp
    src/daemon/DiscordRPCDaemon.cpp
    src/daemon/DiscordRPC.cpp
)

if(WIN32)
    target_sources(discord-drawing-rpc-daemon PRIVATE resources/app_daemon.rc)
    set_target_properties(discord-drawing-rpc-daemon PROPERTIES OUTPUT_NAME "DiscordDrawingRPCDaemon")
endif()

target_link_libraries(discord-drawing-rpc-daemon
    discord_common
)

# Discord RPC GUI
add_executable(discord-drawing-rpc
    src/gui/main.cpp
    src/gui/MainWindow.cpp
    src/gui/SettingsDialog.cpp
    src/gui/CropWidget.cpp
    src/gui/ScreenshotSelector.cpp
    src/gui/LogViewerDialog.cpp
    resources.qrc
)

if(WIN32)
    target_sources(discord-drawing-rpc PRIVATE resources/app_gui.rc)
    set_target_properties(discord-drawing-rpc PROPERTIES OUTPUT_NAME "DiscordDrawingRPC")
endif()

target_link_libraries(discord-drawing-rpc
    discord_common
)

# Discord RPC Tray
add_executable(discord-drawing-rpc-tray
    src/tray/main.cpp
    src/tray/TrayIcon.cpp
    resources.qrc
)

if(WIN32)
    target_sources(discord-drawing-rpc-tray PRIVATE resources/app_tray.rc)
    set_target_properties(discord-drawing-rpc-tray PROPERTIES OUTPUT_NAME "DiscordDrawingRPCTray")
endif()

target_link_libraries(discord-drawing-rpc-tray
    discord_common
    Qt6::Core
    Qt6::Widgets
)
if(WIN32)
    # Windows: Hide console for GUI applications
    set_target_properties(discord-drawing-rpc PROPERTIES WIN32_EXECUTABLE TRUE)
    set_target_properties(discord-drawing-rpc-tray PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# Install targets
install(TARGETS discord-drawing-rpc-daemon discord-drawing-rpc discord-drawing-rpc-tray
    RUNTIME DESTINATION bin
)
install(TARGETS discord_common
    LIBRARY DESTINATION lib
)

# Install Linux desktop files and icons
if(UNIX AND NOT APPLE)
    if(FLATPAK_BUILD)
        # Flatpak: Use Flatpak ID for file names
        install(FILES ${CMAKE_BINARY_DIR}/generated/discord-drawing-rpc.desktop
            DESTINATION share/applications
            RENAME ${FLATPAK_ID}.desktop
        )
        if(EXISTS "${CMAKE_SOURCE_DIR}/icon.png")
            install(FILES icon.png
                DESTINATION share/icons/hicolor/512x512/apps
                RENAME ${FLATPAK_ID}.png
            )
        endif()
        # Install metainfo file
        install(FILES ${CMAKE_BINARY_DIR}/flatpak/${FLATPAK_ID}.metainfo.xml
            DESTINATION share/metainfo
        )
    else()
        # Regular Linux installation
        install(FILES ${CMAKE_BINARY_DIR}/generated/discord-drawing-rpc.desktop
            DESTINATION share/applications
        )
        if(EXISTS "${CMAKE_SOURCE_DIR}/icon.png")
            install(FILES ${CMAKE_SOURCE_DIR}/icon.png
                DESTINATION share/icons/hicolor/512x512/apps
                RENAME discord-drawing-rpc.png
            )
        endif()
    endif()
endif()


# Deploy to dist folder with DLLs
set(DIST_DIR "${CMAKE_BINARY_DIR}/dist")

# Create custom target to deploy executables with dependencies
# For Flatpak builds, the install targets are used instead of the dist folder
if(FLATPAK_BUILD)
    add_custom_target(deploy
        DEPENDS discord-drawing-rpc-daemon discord-drawing-rpc discord-drawing-rpc-tray
        COMMENT "Skipping dist deployment for Flatpak build..."
    )
else()
    add_custom_target(deploy ALL
        DEPENDS discord-drawing-rpc-daemon discord-drawing-rpc discord-drawing-rpc-tray
        COMMENT "Deploying executables and DLLs to dist folder..."
    )
    
    # Copy executables to dist
    add_custom_command(TARGET deploy POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DIST_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:discord_common> ${DIST_DIR}/
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:discord-drawing-rpc-daemon> ${DIST_DIR}/
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:discord-drawing-rpc> ${DIST_DIR}/
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:discord-drawing-rpc-tray> ${DIST_DIR}/
        COMMENT "Copying executables to dist..."
    )

    # Deploy Qt dependencies
    if(WIN32)
        # Find windeployqt
        get_target_property(QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(QT_BIN_DIR "${QMAKE_EXECUTABLE}" DIRECTORY)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BIN_DIR}")
        
        if(WINDEPLOYQT_EXECUTABLE)
            # Deploy Qt DLLs for each executable
            add_custom_command(TARGET deploy POST_BUILD
                COMMAND ${WINDEPLOYQT_EXECUTABLE} --no-translations --no-system-d3d-compiler --no-opengl-sw 
                        "${DIST_DIR}/DiscordDrawingRPC.exe"
                COMMAND ${WINDEPLOYQT_EXECUTABLE} --no-translations --no-system-d3d-compiler --no-opengl-sw 
                        "${DIST_DIR}/DiscordDrawingRPCTray.exe"
                COMMAND ${WINDEPLOYQT_EXECUTABLE} --no-translations --no-system-d3d-compiler --no-opengl-sw 
                        "${DIST_DIR}/DiscordDrawingRPCDaemon.exe"
                COMMENT "Deploying Qt dependencies..."
            )
        else()
            message(WARNING "windeployqt not found. Qt DLLs will need to be copied manually.")
        endif()
        
        # Automatically copy all MinGW DLLs using ldd (only for MinGW builds)
        if(MINGW OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            add_custom_command(TARGET deploy POST_BUILD
                COMMAND bash -c "ldd ${DIST_DIR}/DiscordDrawingRPC.exe | grep /mingw64 | awk '{print $$3}' | xargs -I {} cp -u {} ${DIST_DIR}/ || true"
                COMMAND bash -c "ldd ${DIST_DIR}/DiscordDrawingRPCTray.exe | grep /mingw64 | awk '{print $$3}' | xargs -I {} cp -u {} ${DIST_DIR}/ || true"
                COMMAND bash -c "ldd ${DIST_DIR}/DiscordDrawingRPCDaemon.exe | grep /mingw64 | awk '{print $$3}' | xargs -I {} cp -u {} ${DIST_DIR}/ || true"
                COMMENT "Automatically copying all MinGW DLLs..."
            )
        endif()
    endif()
endif()


# Installer target (Windows only)
if(WIN32)
    add_custom_target(installer
        COMMAND ${CMAKE_COMMAND} 
            -DCMAKE_BINARY_DIR="${CMAKE_BINARY_DIR}"
            -DINSTALLER_SOURCE_DIR="${CMAKE_SOURCE_DIR}/installer"
            -P "${CMAKE_SOURCE_DIR}/installer/build_installer.cmake"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        DEPENDS deploy
        COMMENT "Building Windows installer with Qt Installer Framework..."
    )
    
    message(STATUS "Installer target added. Run 'cmake --build . --target installer' to create the installer.")
endif()

# Clean target to remove build artifacts and generated files
add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}/dist"
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}/packages"
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_SOURCE_DIR}/installer/packages/com.TheGameratorT.app/data"
    COMMAND ${CMAKE_COMMAND} -E remove "${CMAKE_BINARY_DIR}/DiscordDrawingRPC-Setup.exe"
    COMMENT "Cleaning all build artifacts, dist folder, packages, and installer data..."
)
