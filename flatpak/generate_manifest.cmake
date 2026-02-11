# Script to generate Flatpak manifest for CI or Release builds

if(NOT DEFINED MANIFEST_TYPE)
    message(FATAL_ERROR "MANIFEST_TYPE must be defined (CI or PUBLISHING)")
endif()
if(NOT DEFINED APP_VERSION_STRING)
    message(FATAL_ERROR "APP_VERSION_STRING must be defined")
endif()

if(NOT DEFINED SOURCE_DIR)
    message(FATAL_ERROR "SOURCE_DIR must be defined")
endif()

if(NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR "OUTPUT_DIR must be defined")
endif()

# Create output directory if it doesn't exist
file(MAKE_DIRECTORY "${OUTPUT_DIR}")

# Set the sources section based on manifest type
if(MANIFEST_TYPE STREQUAL "CI")
    
    # For CI and release builds: use local directory
    set(FLATPAK_SOURCES "      - type: dir\n        path: ../..")
    set(OUTPUT_FILE "${OUTPUT_DIR}/com.TheGameratorT.DiscordDrawingRPC.yml")
    message(STATUS "Generating CI manifest: ${OUTPUT_FILE}")
    message(STATUS "  Version: ${APP_VERSION_STRING}")
    message(STATUS "  Source: Local directory")
    
elseif(MANIFEST_TYPE STREQUAL "PUBLISHING")

    # For Flathub publishing: use git tag
    set(FLATPAK_SOURCES "      - type: git\n        url: https://github.com/TheGameratorT/discord-drawing-rpc.git\n        tag: v${APP_VERSION_STRING}")
    set(OUTPUT_FILE "${OUTPUT_DIR}/com.TheGameratorT.DiscordDrawingRPC.yml")
    message(STATUS "Generating publishing manifest: ${OUTPUT_FILE}")
    message(STATUS "  Version: ${APP_VERSION_STRING}")
    message(STATUS "  Source: Git tag for Flathub")
    
else()
    message(FATAL_ERROR "Invalid MANIFEST_TYPE: ${MANIFEST_TYPE}. Must be CI or PUBLISHING")
endif()

# Read the template file
file(READ "${SOURCE_DIR}/flatpak/com.TheGameratorT.DiscordDrawingRPC.yml.in" MANIFEST_CONTENT)

# Replace placeholders
string(REPLACE "@APP_VERSION_STRING@" "${APP_VERSION_STRING}" MANIFEST_CONTENT "${MANIFEST_CONTENT}")
string(REPLACE "@FLATPAK_SOURCES@" "${FLATPAK_SOURCES}" MANIFEST_CONTENT "${MANIFEST_CONTENT}")

# Write the output file
file(WRITE "${OUTPUT_FILE}" "${MANIFEST_CONTENT}")

message(STATUS "Manifest generated successfully!")
