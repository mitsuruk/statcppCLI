# =============================================================================
# gflags (Google Commandline Flags) CMake configuration
#
# This file configures gflags library for the project.
# gflags is a C++ library that implements commandline flags processing.
# It includes built-in support for standard types such as string and
# the ability to define flags in the source file in which they are used.
#
# Download directory: ${CMAKE_CURRENT_SOURCE_DIR}/download/gflags
# Install directory:  ${CMAKE_CURRENT_SOURCE_DIR}/download/gflags-install
#
# - If gflags-install/lib/cmake/gflags/gflags-config.cmake already exists, skip download and build.
# - If download/gflags/CMakeLists.txt already exists, skip download (reuse cache).
# - Otherwise, download from GitHub, configure with CMake, build, and install.
#
# License: MIT License (this cmake file)
# Note: gflags library itself is licensed under the BSD 3-Clause License.
# =============================================================================

include_guard(GLOBAL)

message(STATUS "===============================================================")
message(STATUS "gflags configuration:")

# Path to download/install directories
set(GFLAGS_DOWNLOAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/download/gflags)
set(GFLAGS_SOURCE_DIR ${GFLAGS_DOWNLOAD_DIR}/gflags)
set(GFLAGS_INSTALL_DIR ${GFLAGS_DOWNLOAD_DIR}/gflags-install)
set(GFLAGS_BUILD_DIR ${GFLAGS_SOURCE_DIR}/_build)
set(GFLAGS_VERSION "2.2.2")
set(GFLAGS_URL "https://github.com/gflags/gflags/archive/refs/tags/v${GFLAGS_VERSION}.tar.gz")

message(STATUS "GFLAGS_SOURCE_DIR  = ${GFLAGS_SOURCE_DIR}")
message(STATUS "GFLAGS_INSTALL_DIR = ${GFLAGS_INSTALL_DIR}")

# =============================================================================
# gflags Library: Download, Build, and Install (cached in download/ directory)
# =============================================================================
if(EXISTS ${GFLAGS_INSTALL_DIR}/lib/cmake/gflags/gflags-config.cmake)
    message(STATUS "gflags already built: ${GFLAGS_INSTALL_DIR}/lib/cmake/gflags/gflags-config.cmake")
else()
    # --- Download (skip if source already cached) ---
    if(EXISTS ${GFLAGS_SOURCE_DIR}/CMakeLists.txt)
        message(STATUS "gflags source already cached: ${GFLAGS_SOURCE_DIR}")
    else()
        set(GFLAGS_ARCHIVE ${GFLAGS_DOWNLOAD_DIR}/gflags-${GFLAGS_VERSION}.tar.gz)
        set(GFLAGS_URLS
            "https://github.com/gflags/gflags/archive/refs/tags/v${GFLAGS_VERSION}.tar.gz"
        )

        set(GFLAGS_DOWNLOADED FALSE)
        foreach(URL ${GFLAGS_URLS})
            message(STATUS "Downloading gflags ${GFLAGS_VERSION} from ${URL} ...")
            file(DOWNLOAD
                ${URL}
                ${GFLAGS_ARCHIVE}
                SHOW_PROGRESS
                TIMEOUT 300
                INACTIVITY_TIMEOUT 60
                STATUS DOWNLOAD_STATUS
            )
            list(GET DOWNLOAD_STATUS 0 DOWNLOAD_RESULT)
            if(DOWNLOAD_RESULT EQUAL 0)
                set(GFLAGS_DOWNLOADED TRUE)
                break()
            else()
                list(GET DOWNLOAD_STATUS 1 DOWNLOAD_ERROR)
                message(WARNING "Download from ${URL} failed: ${DOWNLOAD_ERROR}")
                file(REMOVE ${GFLAGS_ARCHIVE})
            endif()
        endforeach()

        if(NOT GFLAGS_DOWNLOADED)
            message(FATAL_ERROR
                "gflags download failed.\n"
                "You can manually download and place the file:\n"
                "  curl -L -o download/gflags-${GFLAGS_VERSION}.tar.gz ${GFLAGS_URL}\n"
                "Then re-run cmake."
            )
        endif()

        message(STATUS "Extracting gflags ${GFLAGS_VERSION} ...")
        file(ARCHIVE_EXTRACT
            INPUT ${GFLAGS_ARCHIVE}
            DESTINATION ${GFLAGS_DOWNLOAD_DIR}
        )
        # Rename extracted directory (gflags-2.2.2 -> gflags)
        file(RENAME ${GFLAGS_DOWNLOAD_DIR}/gflags-${GFLAGS_VERSION} ${GFLAGS_SOURCE_DIR})

        message(STATUS "gflags source cached: ${GFLAGS_SOURCE_DIR}")
    endif()

    # --- Configure (CMake) ---
    message(STATUS "Configuring gflags with CMake ...")
    file(MAKE_DIRECTORY ${GFLAGS_BUILD_DIR})

    # ジェネレータ引数リストを構築(-A オプションは Visual Studio のみ有効)
    set(GFLAGS_CMAKE_GENERATOR_ARGS
        -G "${CMAKE_GENERATOR}"
    )
    if(CMAKE_GENERATOR_PLATFORM)
        list(APPEND GFLAGS_CMAKE_GENERATOR_ARGS
            -A "${CMAKE_GENERATOR_PLATFORM}"
        )
    endif()

    execute_process(
        COMMAND ${CMAKE_COMMAND}
                ${GFLAGS_CMAKE_GENERATOR_ARGS}
                -DCMAKE_INSTALL_PREFIX=${GFLAGS_INSTALL_DIR}
                -DBUILD_SHARED_LIBS=OFF
                -DBUILD_STATIC_LIBS=ON
                -DBUILD_TESTING=OFF
                -DBUILD_PACKAGING=OFF
                -DBUILD_gflags_nothreads_LIB=OFF
                -DCMAKE_BUILD_TYPE=Release
                -DCMAKE_POSITION_INDEPENDENT_CODE=ON
                -DCMAKE_POLICY_VERSION_MINIMUM=3.5
                ${GFLAGS_SOURCE_DIR}
        WORKING_DIRECTORY ${GFLAGS_BUILD_DIR}
        RESULT_VARIABLE GFLAGS_CONFIGURE_RESULT
    )
    if(NOT GFLAGS_CONFIGURE_RESULT EQUAL 0)
        message(FATAL_ERROR "gflags CMake configure failed")
    endif()

    # --- Build ---
    message(STATUS "Building gflags (this may take a while) ...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} --build . --config Release -j4
        WORKING_DIRECTORY ${GFLAGS_BUILD_DIR}
        RESULT_VARIABLE GFLAGS_BUILD_RESULT
    )
    if(NOT GFLAGS_BUILD_RESULT EQUAL 0)
        message(FATAL_ERROR "gflags build failed")
    endif()

    # --- Install ---
    message(STATUS "Installing gflags to ${GFLAGS_INSTALL_DIR} ...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} --install . --config Release
        WORKING_DIRECTORY ${GFLAGS_BUILD_DIR}
        RESULT_VARIABLE GFLAGS_INSTALL_RESULT
    )
    if(NOT GFLAGS_INSTALL_RESULT EQUAL 0)
        message(FATAL_ERROR "gflags install failed")
    endif()

    message(STATUS "gflags ${GFLAGS_VERSION} built and installed successfully")
endif()

# =============================================================================
# gflags Library Configuration
# =============================================================================
# Use find_package with the installed gflags CMake config
# GFLAGS_NOTHREADS must be OFF because we built without nothreads variant
# GFLAGS_USE_TARGET_NAMESPACE enables gflags::gflags target name
set(GFLAGS_NOTHREADS OFF)
set(GFLAGS_USE_TARGET_NAMESPACE TRUE)
set(gflags_DIR ${GFLAGS_INSTALL_DIR}/lib/cmake/gflags)
find_package(gflags REQUIRED CONFIG)

# Link gflags to the project (gflags::gflags carries include dirs and compile defs)
target_link_libraries(${PROJECT_NAME} PRIVATE gflags::gflags)

message(STATUS "gflags linked to ${PROJECT_NAME}")
message(STATUS "===============================================================")
