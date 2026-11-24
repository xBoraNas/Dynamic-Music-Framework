cmake_minimum_required(VERSION 3.21)

# vcpkg toolchain yolunu CMake Anbelle�Yi silinse bile otomatik bul
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "C:/Program Files/Microsoft Visual Studio/18/Community/VC/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(
    DynamicMusicFramework
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)

include(FetchContent)

set(ENABLE_SKYRIM_SE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_AE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_VR OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(RAPIDCSV_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/include" CACHE PATH "" FORCE)

FetchContent_Declare(
    CommonLibSSE
    GIT_REPOSITORY https://github.com/alandtse/CommonLibSSE-NG.git
    GIT_TAG 5298302831d03a01b3b406f4b7b8128ee3bd7a44
)
FetchContent_MakeAvailable(CommonLibSSE)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_library(${PROJECT_NAME} SHARED
    src/main.cpp
    src/ConfigManager.cpp
    src/MenuUI.cpp
    src/AudioEngine.cpp
    src/MusicDetector.cpp
    src/AmbientDetector.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/external
    ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/include
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    UNICODE
    _UNICODE
)

target_compile_options(${PROJECT_NAME} PRIVATE
    /utf-8
    /W3
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    <RE/Skyrim.h>
    <SKSE/SKSE.h>
    <spdlog/spdlog.h>
    <nlohmann/json.hpp>
    <string_view>
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    CommonLibSSE::CommonLibSSE
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)