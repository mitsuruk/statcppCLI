# =============================================================================
# nlohmann-json CMake configuration
#
# This file configures the nlohmann/json header-only library for the project.
# nlohmann/json is a modern C++ JSON library providing intuitive syntax,
# STL-like access, serialization/deserialization, and more.
#
# Download directory: ${CMAKE_CURRENT_SOURCE_DIR}/download/nlohmann-json
# Install directory:  ${CMAKE_CURRENT_SOURCE_DIR}/download/nlohmann-json-install
#
# - If nlohmann-json-install/include/nlohmann/json.hpp already exists,
#   skip download (reuse cache).
# - Otherwise, download json.hpp from GitHub and install.
#
# Note: nlohmann/json is header-only. No compilation or linking is needed.
#
# License: MIT License (this cmake file)
# Note: nlohmann/json library itself is licensed under MIT License.
# =============================================================================

include_guard(GLOBAL)

message(STATUS "===============================================================")
message(STATUS "nlohmann-json configuration:")

# Path to download/install directories
set(NLOHMANN_JSON_DOWNLOAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/download/nlohmann-json)
set(NLOHMANN_JSON_INSTALL_DIR ${NLOHMANN_JSON_DOWNLOAD_DIR}/nlohmann-json-install)
set(NLOHMANN_JSON_VERSION "3.12.0")
set(NLOHMANN_JSON_URL "https://github.com/nlohmann/json/releases/download/v${NLOHMANN_JSON_VERSION}/json.hpp")

message(STATUS "NLOHMANN_JSON_INSTALL_DIR = ${NLOHMANN_JSON_INSTALL_DIR}")

# =============================================================================
# nlohmann-json: Download and Install (cached in download/ directory)
# =============================================================================
if(EXISTS ${NLOHMANN_JSON_INSTALL_DIR}/include/nlohmann/json.hpp)
    message(STATUS "nlohmann-json already installed: ${NLOHMANN_JSON_INSTALL_DIR}/include/nlohmann/json.hpp")
else()
    # Create install directory structure
    file(MAKE_DIRECTORY ${NLOHMANN_JSON_INSTALL_DIR}/include/nlohmann)

    # Check if json.hpp is already cached in download/
    set(NLOHMANN_JSON_CACHED ${NLOHMANN_JSON_DOWNLOAD_DIR}/json.hpp)

    if(EXISTS ${NLOHMANN_JSON_CACHED})
        message(STATUS "nlohmann-json source already cached: ${NLOHMANN_JSON_CACHED}")
    else()
        message(STATUS "Downloading nlohmann-json ${NLOHMANN_JSON_VERSION} from GitHub ...")
        file(DOWNLOAD
            ${NLOHMANN_JSON_URL}
            ${NLOHMANN_JSON_CACHED}
            SHOW_PROGRESS
            TIMEOUT 120
            INACTIVITY_TIMEOUT 30
            STATUS DOWNLOAD_STATUS
        )
        list(GET DOWNLOAD_STATUS 0 DOWNLOAD_RESULT)
        if(NOT DOWNLOAD_RESULT EQUAL 0)
            list(GET DOWNLOAD_STATUS 1 DOWNLOAD_ERROR)
            file(REMOVE ${NLOHMANN_JSON_CACHED})
            message(FATAL_ERROR
                "nlohmann-json download failed: ${DOWNLOAD_ERROR}\n"
                "You can manually download and place the file:\n"
                "  curl -L -o download/json.hpp ${NLOHMANN_JSON_URL}\n"
                "Then re-run cmake."
            )
        endif()
    endif()

    # Copy to install directory
    file(COPY ${NLOHMANN_JSON_CACHED}
        DESTINATION ${NLOHMANN_JSON_INSTALL_DIR}/include/nlohmann
    )

    # Verify installation
    if(NOT EXISTS ${NLOHMANN_JSON_INSTALL_DIR}/include/nlohmann/json.hpp)
        message(FATAL_ERROR "nlohmann-json installation failed")
    endif()

    message(STATUS "nlohmann-json ${NLOHMANN_JSON_VERSION} installed successfully")
endif()

# =============================================================================
# nlohmann-json Library Configuration
# =============================================================================
# Header-only: just add include directory, no linking needed
target_include_directories(${PROJECT_NAME} PRIVATE
    ${NLOHMANN_JSON_INSTALL_DIR}/include
)

message(STATUS "nlohmann-json headers added to ${PROJECT_NAME}")
message(STATUS "===============================================================")
