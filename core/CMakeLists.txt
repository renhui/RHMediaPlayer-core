cmake_minimum_required(VERSION 3.4.1)

set(lib_src_DIR ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI})

include_directories(
        ${CMAKE_SOURCE_DIR}/src/main/cpp/include
        ${CMAKE_SOURCE_DIR}/src/main/cpp/SoundTouch
)

add_library(avcodec-57_lib SHARED IMPORTED)
set_target_properties(avcodec-57_lib PROPERTIES IMPORTED_LOCATION
                     ${lib_src_DIR}/libavcodec-57.so)

add_library(avformat-57_lib SHARED IMPORTED)
set_target_properties(avformat-57_lib PROPERTIES IMPORTED_LOCATION
                        ${lib_src_DIR}/libavformat-57.so)

add_library(avutil-55_lib SHARED IMPORTED)
set_target_properties(avutil-55_lib PROPERTIES IMPORTED_LOCATION
                        ${lib_src_DIR}/libavutil-55.so)

add_library(swresample-2_lib SHARED IMPORTED)
set_target_properties(swresample-2_lib PROPERTIES IMPORTED_LOCATION
                        ${lib_src_DIR}/libswresample-2.so)

add_library(swscale-4_lib SHARED IMPORTED)
set_target_properties(swscale-4_lib PROPERTIES IMPORTED_LOCATION
                        ${lib_src_DIR}/libswscale-4.so)

add_library(postproc-54_lib SHARED IMPORTED)
set_target_properties(postproc-54_lib PROPERTIES IMPORTED_LOCATION
                        ${lib_src_DIR}/libpostproc-54.so)

add_library(avdevice-57_lib SHARED IMPORTED)
set_target_properties(avdevice-57_lib PROPERTIES IMPORTED_LOCATION
                        ${lib_src_DIR}/libavdevice-57.so)

add_library(avfilter-6_lib SHARED IMPORTED)
set_target_properties(avfilter-6_lib PROPERTIES IMPORTED_LOCATION
                        ${lib_src_DIR}/libavfilter-6.so)

# build application's shared lib
# For Java Call, the file of C/C++ Use UpperCase Name Space

add_library(media-editor-lib SHARED
            ${CMAKE_SOURCE_DIR}/src/main/cpp/player/PlayerStatus.cpp
            ${CMAKE_SOURCE_DIR}/src/main/cpp/player/BasePlayer.cpp
            ${CMAKE_SOURCE_DIR}/src/main/cpp/player/JavaCall.cpp
            ${CMAKE_SOURCE_DIR}/src/main/cpp/player/FFmpegPlayer.cpp
            ${CMAKE_SOURCE_DIR}/src/main/cpp/player/AudioChannel.cpp
            ${CMAKE_SOURCE_DIR}/src/main/cpp/player/PlayerQueue.cpp
            ${CMAKE_SOURCE_DIR}/src/main/cpp/player/FFmpegHandler.cpp
            ${CMAKE_SOURCE_DIR}/src/main/cpp/player/AudioHandler.cpp
            ${CMAKE_SOURCE_DIR}/src/main/cpp/player/VideoHandler.cpp

            # SoundTouch Library
            ${CMAKE_SOURCE_DIR}/src/main/cpp/SoundTouch/AAFilter.cpp
            ${CMAKE_SOURCE_DIR}/src/main/cpp/SoundTouch/FIFOSampleBuffer.cpp
            ${CMAKE_SOURCE_DIR}/src/main/cpp/SoundTouch/FIRFilter.cpp
            ${CMAKE_SOURCE_DIR}/src/main/cpp/SoundTouch/cpu_detect_x86.cpp
            ${CMAKE_SOURCE_DIR}/src/main/cpp/SoundTouch/sse_optimized.cpp
            ${CMAKE_SOURCE_DIR}/src/main/cpp/SoundTouch/RateTransposer.cpp
            ${CMAKE_SOURCE_DIR}/src/main/cpp/SoundTouch/SoundTouch.cpp
            ${CMAKE_SOURCE_DIR}/src/main/cpp/SoundTouch/InterpolateCubic.cpp
            ${CMAKE_SOURCE_DIR}/src/main/cpp/SoundTouch/InterpolateLinear.cpp
            ${CMAKE_SOURCE_DIR}/src/main/cpp/SoundTouch/InterpolateShannon.cpp
            ${CMAKE_SOURCE_DIR}/src/main/cpp/SoundTouch/TDStretch.cpp
            ${CMAKE_SOURCE_DIR}/src/main/cpp/SoundTouch/BPMDetect.cpp
            ${CMAKE_SOURCE_DIR}/src/main/cpp/SoundTouch/PeakFinder.cpp
           )

# Include libraries needed for MediaEditor lib
target_link_libraries(media-editor-lib
log
android
OpenSLES
avcodec-57_lib
avdevice-57_lib
avfilter-6_lib
avformat-57_lib
avutil-55_lib
postproc-54_lib
swresample-2_lib
swscale-4_lib)
