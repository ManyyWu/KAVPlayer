cmake_minimum_required(VERSION 3.10)

project(KAVPlayer)

set(CMAKE_PREFIX_PATH /home/openkt/Qt5.9.1/5.9.1/gcc_64/lib/cmake)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -lSDL_ttf -lfreetype -lavformat -pthread -ldl")
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Gui REQUIRED)

include_directories(src/FFmpeg/4.0.2/include)
include_directories(src/SDL2/2.0.9/include)
include_directories(src/clock)
include_directories(src/decoder)
include_directories(src/demux)
include_directories(src/error)
include_directories(src/infile)
include_directories(src/log)
include_directories(src/msger)
include_directories(src/queue)
include_directories(src/render)
include_directories(src/utils)
include_directories(src/vdev)
include_directories(src)
link_directories(src/FFmpeg/4.0.2/lib)
link_directories(src/SDL2/2.0.9/lib)

set(SRC_FILES src/adev/adev.cpp
              src/adev/adev.h
              src/clock/clock.cpp
              src/clock/clock.h
              src/decoder/decoder.cpp
              src/decoder/decoder.h
              src/demux/demux.cpp
              src/demux/demux.h
              src/error/error.cpp
              src/error/error.h
              src/inifile/inifile.cpp
              src/inifile/inifile.h
              src/log/log.cpp
              src/log/log.h
              src/msger/msger.cpp
              src/msger/msger.h
              src/queue/frame_queue.cpp
              src/queue/frame_queue.h
              src/queue/packet_queue.cpp
              src/queue/packet_queue.h
              src/render/render.cpp
              src/render/render.h
              src/utils/utils.cpp
              src/utils/utils.h
              src/vdev/vdev.cpp
              src/vdev/vdev.h
              src/avplayerwidget_global.h
              src/AVPlayerWidget.cpp
              src/AVPlayerWidget.h
              src/KAVPlayer.cpp
              src/KAVPlayer.h
              src/main.cpp
              )

add_executable(KAVPlayer ${SRC_FILES})

target_link_libraries(KAVPlayer
                      Qt5::Core
                      Qt5::Widgets
                      Qt5::Gui
                      libavcodec.a
                      libavformat.a
                      libswresample.a
                      libswscale.a
                      libavutil.a
                      libSDL2.a
                      libSDL2main.a
                      libSDL_ttf.a
                      )
