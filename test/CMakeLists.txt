# =============================================================================
# statcppCLI - Google Test Configuration
#
# 独立ビルド対応: cmake -B build -S test で直接ビルド可能
# add_subdirectory 経由でも動作する
# =============================================================================

# 独立ビルド対応（CI等で -S test として呼ばれた場合）
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.20)
    project(statcppcli_test LANGUAGES CXX)
    enable_testing()
    set(STANDALONE_TEST ON)
endif()

# ソースルートディレクトリ（独立ビルド時は親ディレクトリ）
if(STANDALONE_TEST)
    set(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
else()
    set(REPO_ROOT ${PROJECT_SOURCE_DIR})
endif()

# テストターゲット名
set(TEST_TARGET_NAME statcppcli_test)

# テストソースファイル
set(TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test_cli_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_csv_reader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_output_formatter.cpp
)

# テスト実行ファイルを作成
add_executable(${TEST_TARGET_NAME} ${TEST_SOURCES})

# C++17を使用
target_compile_features(${TEST_TARGET_NAME} PRIVATE cxx_std_17)

# インクルードディレクトリ
target_include_directories(${TEST_TARGET_NAME} PRIVATE ${REPO_ROOT}/src/include)

# 依存ヘッダのインクルードパス（独立ビルド時）
if(STANDALONE_TEST)
    # statcpp（csv_reader.hpp が依存）
    # CI では STATCPP_INCLUDE_DIR を外部から渡す
    if(DEFINED STATCPP_INCLUDE_DIR)
        target_include_directories(${TEST_TARGET_NAME} PRIVATE ${STATCPP_INCLUDE_DIR})
    elseif(EXISTS ${REPO_ROOT}/download/statcpp/statcpp-install/include)
        target_include_directories(${TEST_TARGET_NAME} PRIVATE
            ${REPO_ROOT}/download/statcpp/statcpp-install/include)
    else()
        message(FATAL_ERROR "statcpp headers not found. Set -DSTATCPP_INCLUDE_DIR=<path>")
    endif()

    # nlohmann-json（output_formatter.hpp が依存）
    set(NLOHMANN_JSON_DOWNLOAD_DIR ${REPO_ROOT}/download/nlohmann-json)
    set(NLOHMANN_JSON_INSTALL_DIR ${NLOHMANN_JSON_DOWNLOAD_DIR}/nlohmann-json-install)
    set(NLOHMANN_JSON_VERSION "3.12.0")
    set(NLOHMANN_JSON_URL "https://github.com/nlohmann/json/releases/download/v${NLOHMANN_JSON_VERSION}/json.hpp")

    if(NOT EXISTS ${NLOHMANN_JSON_INSTALL_DIR}/include/nlohmann/json.hpp)
        file(MAKE_DIRECTORY ${NLOHMANN_JSON_INSTALL_DIR}/include/nlohmann)
        set(NLOHMANN_JSON_CACHED ${NLOHMANN_JSON_DOWNLOAD_DIR}/json.hpp)
        if(NOT EXISTS ${NLOHMANN_JSON_CACHED})
            message(STATUS "Downloading nlohmann-json ${NLOHMANN_JSON_VERSION} ...")
            file(DOWNLOAD ${NLOHMANN_JSON_URL} ${NLOHMANN_JSON_CACHED}
                SHOW_PROGRESS TIMEOUT 120 STATUS DOWNLOAD_STATUS)
            list(GET DOWNLOAD_STATUS 0 DOWNLOAD_RESULT)
            if(NOT DOWNLOAD_RESULT EQUAL 0)
                file(REMOVE ${NLOHMANN_JSON_CACHED})
                message(FATAL_ERROR "nlohmann-json download failed")
            endif()
        endif()
        file(COPY ${NLOHMANN_JSON_CACHED}
            DESTINATION ${NLOHMANN_JSON_INSTALL_DIR}/include/nlohmann)
    endif()

    target_include_directories(${TEST_TARGET_NAME} PRIVATE
        ${NLOHMANN_JSON_INSTALL_DIR}/include
    )
endif()

# Google Testをリンク
find_package(GTest REQUIRED)
target_link_libraries(${TEST_TARGET_NAME}
    PRIVATE
    GTest::gtest
    GTest::gtest_main
)

# CTestに登録
include(GoogleTest)
gtest_discover_tests(${TEST_TARGET_NAME})

# コンパイルオプション
target_compile_options(${TEST_TARGET_NAME} PRIVATE
    -Wall
    -finput-charset=UTF-8
    -fexec-charset=UTF-8
)

# 出力ディレクトリ
set_target_properties(${TEST_TARGET_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
