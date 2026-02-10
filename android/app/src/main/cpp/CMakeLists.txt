cmake_minimum_required(VERSION 3.22.1)

project("img2ascii")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_library(log-lib log)
find_library(android-lib android)
find_library(EGL-lib EGL)
find_library(GLESv2-lib GLESv2)
find_library(camera2ndk camera2ndk)
find_library(mediandk mediandk)

include_directories(
    ../../../../../src
)

add_library(img2ascii SHARED
    android_main.cpp
    android_renderer.cpp
    ../../../../../src/ascii_converter.cpp
    android_camera.cpp
)

target_link_libraries(img2ascii
    ${log-lib}
    ${android-lib}
    ${EGL-lib}
    ${GLESv2-lib}
    ${camera2ndk}
    ${mediandk}
)