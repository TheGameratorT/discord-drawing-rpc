# CMake script to build the installer
# This script packages the dist folder and creates the installer

cmake_minimum_required(VERSION 3.16)

# Verify required variables are set
if(NOT CMAKE_BINARY_DIR)
    message(FATAL_ERROR "CMAKE_BINARY_DIR must be set when running this script")
endif()

if(NOT INSTALLER_SOURCE_DIR)
    message(FATAL_ERROR "INSTALLER_SOURCE_DIR must be set when running this script")
endif()

# Find binarycreator from Qt Installer Framework
# On MSYS2, it's installed via mingw-w64-x86_64-qt-installer-framework package
find_program(BINARYCREATOR_EXECUTABLE
    NAMES binarycreator binarycreator.exe
    PATHS 
        ENV QTIFWDIR
    PATH_SUFFIXES bin
    DOC "Qt Installer Framework binarycreator executable"
)

if(NOT BINARYCREATOR_EXECUTABLE)
    message(FATAL_ERROR "binarycreator not found! Please install Qt Installer Framework and set QTIFWDIR environment variable.")
endif()

message(STATUS "Found binarycreator: ${BINARYCREATOR_EXECUTABLE}")

# Set paths
set(INSTALLER_DIR "${INSTALLER_SOURCE_DIR}")
set(DIST_DIR "${CMAKE_BINARY_DIR}/dist")
set(DATA_DIR "${INSTALLER_DIR}/packages/com.TheGameratorT.app/data")
set(META_DIR_BUILD "${CMAKE_BINARY_DIR}/installer/packages/com.TheGameratorT.app/meta")
set(CONFIG_DIR_BUILD "${CMAKE_BINARY_DIR}/installer/config")
set(OUTPUT_DIR "${CMAKE_BINARY_DIR}")

# Clean and prepare data directory
message(STATUS "Cleaning data directory...")
file(REMOVE_RECURSE "${DATA_DIR}")
file(MAKE_DIRECTORY "${DATA_DIR}")

# Copy dist folder contents to installer data directory
message(STATUS "Copying dist contents to installer data directory...")
file(GLOB_RECURSE DIST_FILES "${DIST_DIR}/*")
foreach(DIST_FILE ${DIST_FILES})
    file(RELATIVE_PATH REL_FILE "${DIST_DIR}" "${DIST_FILE}")
    get_filename_component(REL_DIR "${REL_FILE}" DIRECTORY)
    file(MAKE_DIRECTORY "${DATA_DIR}/${REL_DIR}")
    file(COPY "${DIST_FILE}" DESTINATION "${DATA_DIR}/${REL_DIR}")
endforeach()

# Copy configured package.xml from build directory
message(STATUS "Copying configured package.xml...")
if(EXISTS "${META_DIR_BUILD}/package.xml")
    file(COPY "${META_DIR_BUILD}/package.xml" 
         DESTINATION "${INSTALLER_DIR}/packages/com.TheGameratorT.app/meta/")
else()
    message(WARNING "Configured package.xml not found at ${META_DIR_BUILD}/package.xml")
endif()

# Copy configured config.xml from build directory
message(STATUS "Copying configured config.xml...")
if(EXISTS "${CONFIG_DIR_BUILD}/config.xml")
    file(COPY "${CONFIG_DIR_BUILD}/config.xml" 
         DESTINATION "${INSTALLER_DIR}/config/")
else()
    message(WARNING "Configured config.xml not found at ${CONFIG_DIR_BUILD}/config.xml")
endif()

# Create the installer
message(STATUS "Creating installer...")
execute_process(
    COMMAND "${BINARYCREATOR_EXECUTABLE}"
        --offline-only
        -c "${INSTALLER_DIR}/config/config.xml"
        -p "${INSTALLER_DIR}/packages"
        "${OUTPUT_DIR}/DiscordDrawingRPC-Setup.exe"
    WORKING_DIRECTORY "${INSTALLER_DIR}"
    RESULT_VARIABLE INSTALLER_RESULT
    OUTPUT_VARIABLE INSTALLER_OUTPUT
    ERROR_VARIABLE INSTALLER_ERROR
)

if(NOT INSTALLER_RESULT EQUAL 0)
    message(FATAL_ERROR "Installer creation failed!\nOutput: ${INSTALLER_OUTPUT}\nError: ${INSTALLER_ERROR}")
else()
    message(STATUS "Installer created successfully: ${OUTPUT_DIR}/DiscordDrawingRPC-Setup.exe")
endif()
