cmake_minimum_required(VERSION 3.16)
project(DiscordDrawRPC VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
)

target_link_libraries(discord_common
    PUBLIC Qt6::Core Qt6::Widgets Qt6::Network
)

# Discord RPC Daemon
add_executable(DiscordDrawRPCDaemon
    src/daemon/main.cpp
    src/daemon/DiscordRPCDaemon.cpp
    src/daemon/DiscordRPC.cpp
)

if(WIN32)
    target_sources(DiscordDrawRPCDaemon PRIVATE resources/app_daemon.rc)
endif()

target_link_libraries(DiscordDrawRPCDaemon
    discord_common
)

# Discord RPC GUI
add_executable(DiscordDrawRPC
    src/gui/main.cpp
    src/gui/MainWindow.cpp
    src/gui/SettingsDialog.cpp
    src/gui/CropWidget.cpp
    src/gui/ScreenshotSelector.cpp
    src/gui/LogViewerDialog.cpp
    resources.qrc
)

if(WIN32)
    target_sources(DiscordDrawRPC PRIVATE resources/app_gui.rc)
endif()

target_link_libraries(DiscordDrawRPC
    discord_common
)

# Discord RPC Tray
add_executable(DiscordDrawRPCTray
    src/tray/main.cpp
    src/tray/TrayIcon.cpp
    resources.qrc
)

if(WIN32)
    target_sources(DiscordDrawRPCTray PRIVATE resources/app_tray.rc)
endif()

target_link_libraries(DiscordDrawRPCTray
    discord_common
    Qt6::Core
    Qt6::Widgets
)
if(WIN32)
    # Windows: Hide console for GUI applications
    set_target_properties(DiscordDrawRPC PROPERTIES WIN32_EXECUTABLE TRUE)
    set_target_properties(DiscordDrawRPCTray PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# Install targets
install(TARGETS DiscordDrawRPCDaemon DiscordDrawRPC DiscordDrawRPCTray
    RUNTIME DESTINATION bin
)

# Deploy to dist folder with DLLs
set(DIST_DIR "${CMAKE_BINARY_DIR}/dist")

# Create custom target to deploy executables with dependencies
add_custom_target(deploy ALL
    DEPENDS DiscordDrawRPCDaemon DiscordDrawRPC DiscordDrawRPCTray
    COMMENT "Deploying executables and DLLs to dist folder..."
)

# Copy executables to dist
add_custom_command(TARGET deploy POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DIST_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:discord_common> ${DIST_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:DiscordDrawRPCDaemon> ${DIST_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:DiscordDrawRPC> ${DIST_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:DiscordDrawRPCTray> ${DIST_DIR}/
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
                    "${DIST_DIR}/DiscordDrawRPC.exe"
            COMMAND ${WINDEPLOYQT_EXECUTABLE} --no-translations --no-system-d3d-compiler --no-opengl-sw 
                    "${DIST_DIR}/DiscordDrawRPCTray.exe"
            COMMAND ${WINDEPLOYQT_EXECUTABLE} --no-translations --no-system-d3d-compiler --no-opengl-sw 
                    "${DIST_DIR}/DiscordDrawRPCDaemon.exe"
            COMMENT "Deploying Qt dependencies..."
        )
    else()
        message(WARNING "windeployqt not found. Qt DLLs will need to be copied manually.")
    endif()
    
    # Automatically copy all MinGW DLLs using ldd (only for MinGW builds)
    if(MINGW OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_custom_command(TARGET deploy POST_BUILD
            COMMAND bash -c "ldd ${DIST_DIR}/DiscordDrawRPC.exe | grep /mingw64 | awk '{print $$3}' | xargs -I {} cp -u {} ${DIST_DIR}/ || true"
            COMMAND bash -c "ldd ${DIST_DIR}/DiscordDrawRPCTray.exe | grep /mingw64 | awk '{print $$3}' | xargs -I {} cp -u {} ${DIST_DIR}/ || true"
            COMMAND bash -c "ldd ${DIST_DIR}/DiscordDrawRPCDaemon.exe | grep /mingw64 | awk '{print $$3}' | xargs -I {} cp -u {} ${DIST_DIR}/ || true"
            COMMENT "Automatically copying all MinGW DLLs..."
        )
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
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_SOURCE_DIR}/installer/packages/com.discorddrawrpc.app/data"
    COMMAND ${CMAKE_COMMAND} -E remove "${CMAKE_BINARY_DIR}/DiscordDrawRPC-Setup.exe"
    COMMENT "Cleaning all build artifacts, dist folder, packages, and installer data..."
)
