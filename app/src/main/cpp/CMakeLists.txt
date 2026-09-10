cmake_minimum_required(VERSION 3.22.1)
project(dinocrisis_native LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_library(log-lib log)
find_library(android-lib android)
find_library(gles-lib GLESv3)

add_library(dinocrisis_native SHARED
    native_bridge.cpp
    engine/native_engine.cpp
    data/game_data.cpp
    platform/android_filesystem.cpp
    platform/android_input.cpp
    platform/android_audio.cpp
    platform/android_timing.cpp
    platform/android_graphics.cpp
    platform/android_memory.cpp
    ps1/ps1_facade.cpp
)

target_include_directories(dinocrisis_native PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/data
    ${CMAKE_CURRENT_SOURCE_DIR}/platform
    ${CMAKE_CURRENT_SOURCE_DIR}/ps1
)

target_compile_options(dinocrisis_native PRIVATE -Wall -Wextra)

target_link_libraries(dinocrisis_native PRIVATE
    ${log-lib}
    ${android-lib}
    ${gles-lib}
)