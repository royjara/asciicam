cmake_minimum_required(VERSION 3.4.1)

project("img2ascii")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_library(log-lib log)
find_library(android-lib android)
find_library(EGL-lib EGL)
find_library(GLESv2-lib GLESv2)
find_library(camera2ndk camera2ndk)
find_library(mediandk mediandk)

# GStreamer setup
set(GSTREAMER_ROOT_ANDROID ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../gstreamer-1.0-android-universal-1.28.0)
set(GSTREAMER_ROOT ${GSTREAMER_ROOT_ANDROID}/arm64)

# GStreamer static plugin initialization handled by gst_android_init.c

include_directories(
    ../../../../../src
    ${GSTREAMER_ROOT}/include/gstreamer-1.0
    ${GSTREAMER_ROOT}/include/glib-2.0
    ${GSTREAMER_ROOT}/lib/glib-2.0/include
)

add_library(img2ascii SHARED
    android_main.cpp
    android_renderer.cpp
    ../../../../../src/ascii_converter.cpp
    android_camera.cpp
    gstreamer_rtsp_server.cpp
    gst_android_init.c
)

# GStreamer plugins
# Direct library paths - simpler and more reliable approach
set(GSTREAMER_PLUGIN_PATHS
    ${GSTREAMER_ROOT}/lib/gstreamer-1.0/libgstcoreelements.a
    ${GSTREAMER_ROOT}/lib/gstreamer-1.0/libgstapp.a
    ${GSTREAMER_ROOT}/lib/gstreamer-1.0/libgstrtp.a
    ${GSTREAMER_ROOT}/lib/gstreamer-1.0/libgstrtpmanager.a
    ${GSTREAMER_ROOT}/lib/gstreamer-1.0/libgstudp.a
    ${GSTREAMER_ROOT}/lib/gstreamer-1.0/libgstvideoconvertscale.a
    ${GSTREAMER_ROOT}/lib/gstreamer-1.0/libgstx264.a
    ${GSTREAMER_ROOT}/lib/gstreamer-1.0/libgstplayback.a
    ${GSTREAMER_ROOT}/lib/gstreamer-1.0/libgsttypefindfunctions.a
    ${GSTREAMER_ROOT}/lib/gstreamer-1.0/libgstgio.a
)

set(GSTREAMER_CORE_PATHS
    ${GSTREAMER_ROOT}/lib/libgstrtspserver-1.0.a
    ${GSTREAMER_ROOT}/lib/libgstrtsp-1.0.a
    ${GSTREAMER_ROOT}/lib/libgstvideo-1.0.a
    ${GSTREAMER_ROOT}/lib/libgstapp-1.0.a
    ${GSTREAMER_ROOT}/lib/libgstaudio-1.0.a
    ${GSTREAMER_ROOT}/lib/libgstnet-1.0.a
    ${GSTREAMER_ROOT}/lib/libgsttag-1.0.a
    ${GSTREAMER_ROOT}/lib/libgstpbutils-1.0.a
    ${GSTREAMER_ROOT}/lib/libgstrtp-1.0.a
    ${GSTREAMER_ROOT}/lib/libgstsdp-1.0.a
    ${GSTREAMER_ROOT}/lib/libgstcontroller-1.0.a
    ${GSTREAMER_ROOT}/lib/libgstreamer-1.0.a
    ${GSTREAMER_ROOT}/lib/libgstbase-1.0.a
)

set(GSTREAMER_SYSTEM_PATHS
    ${GSTREAMER_ROOT}/lib/libglib-2.0.a
    ${GSTREAMER_ROOT}/lib/libgobject-2.0.a
    ${GSTREAMER_ROOT}/lib/libgmodule-2.0.a
    ${GSTREAMER_ROOT}/lib/libgio-2.0.a
    ${GSTREAMER_ROOT}/lib/libffi.a
    ${GSTREAMER_ROOT}/lib/libiconv.a
    ${GSTREAMER_ROOT}/lib/libintl.a
    ${GSTREAMER_ROOT}/lib/libpcre2-8.a
    ${GSTREAMER_ROOT}/lib/liborc-0.4.a
    ${GSTREAMER_ROOT}/lib/libz.a
    ${GSTREAMER_ROOT}/lib/libx264.a
)

target_link_libraries(img2ascii
    ${log-lib}
    ${android-lib}
    ${EGL-lib}
    ${GLESv2-lib}
    ${camera2ndk}
    ${mediandk}
    -Wl,--start-group
    ${GSTREAMER_PLUGIN_PATHS}
    ${GSTREAMER_CORE_PATHS}
    ${GSTREAMER_SYSTEM_PATHS}
    -Wl,--end-group
)