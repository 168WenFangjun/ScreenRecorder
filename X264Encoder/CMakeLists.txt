cmake_minimum_required(VERSION 2.8.12)
SET(CMAKE_AUTOMOC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)
SET(CMAKE_CXX_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}" )
SET(CMAKE_CXX_STANDARD 14)

SET(PROJECT_NAME X264Encoder)
PROJECT(${PROJECT_NAME})

#SET(HOME_DIR "/Users/oosman")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(_MACHINE_ARCH_FLAG "/machine:x86")
#    SET(CMAKE_PREFIX_PATH
#        ${HOME_DIR}/Qt5/5.5/clang_64/lib/cmake
#    )
    ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
#    SET(CMAKE_PREFIX_PATH
#        ${HOME_DIR}/Qt/5.10.0/gcc_64/lib/cmake
#    )
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

SET(CMAKE_INCLUDE_CURRENT_DIR ON)
SET(CMAKE_AUTOMOC ON)

FIND_PACKAGE(Qt5Core REQUIRED)

message(status "############## " ${HOME_DIR} " ####")

INCLUDE_DIRECTORIES(
    ${QT_INCLUDES}
    ${Qt5Core_INCLUDE_DIRS}
    ../../libs/ffmpeg
    ../../libs/x264
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)
ADD_DEFINITIONS(
    -DHAVE_STDINT_H
)
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    LINK_DIRECTORIES(
        /usr/lib
        ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/build/linux/bin/
        /usr/local/bin/bf-booby-screen-recorder/X264Encoder/
    )
     SET(FFMpegLibs
        avcodec
        avdevice
        avformat
        avfilter
        avutil
        swresample
        swscale
    )
    ADD_DEFINITIONS(
        -DLinux
    )
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    LINK_DIRECTORIES(
        ${HOME_DIR}/work/libs/build/osx/bin
    )
     SET(FFMpegLibs
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        swresample
        swscale
    )
    ADD_DEFINITIONS(
        -DDarwin
    )

ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    ADD_DEFINITIONS(
        -DWin64
    )
    LINK_DIRECTORIES(
        ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/build/win64
    )
     SET(FFMpegLibs
        avcodec-58
        avdevice-58
        avfilter-7
        avformat-58
        avutil-56
        swresample-3
        swscale-5
        x264
    )
    SET(PLATFORM WIN32)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

SET(PROJECT_SOURCES
    X264Encoder.h
    X264Encoder.cpp
    YuvWriter.h
    YuvWriter.cpp
    ../ffwrite.h
    ../ffwrite.c
    FFMpegRunner.h
    FFMpegRunner.cpp
    main.cpp
)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
SET(CMAKE_INSTALL_RPATH "/usr/local/bin/bf-booby-screen-recorder")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

ADD_EXECUTABLE(${PROJECT_NAME} WIN32 MACOSX_BUNDLE
    ${PROJECT_SOURCES}
)
TARGET_LINK_LIBRARIES(${PROJECT_NAME}
    ${Qt5Core_LIBRARIES}
    ${FFMpegLibs}
)
IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS -LC:/work/libs/build/win64/X264Encoder)
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS -Wl,-subsystem,windows)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS -L/usr/local/bin/bf-booby-screen-recorder/X264Encoder)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES MACOSX_BUNDLE TRUE)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

