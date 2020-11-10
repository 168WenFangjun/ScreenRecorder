#to build on MacOS/Ubuntu from command line
#mkdir ../ScreenRecorder-build
#./ccmake ../ScreenRecorder
# c to configure, get some errors, copy name of missing Qt package, exit, find ~/ -name Qt5CoreConfig.cmake, copy path, retry
# after compiling code, to run application, in ScreenRecorder-build:
#ln -s /home/dev/oosman/Qt5.12.1/Tools/QtCreator/lib/Qt/plugins/platforms platforms
#
# CMAKE_OSX_SYSROOT 
#/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.10.sdk
#( you can get SDKs from https://github.com/phracker/MacOSX-SDKs/releases )
# Qt5Core_DIR /Users/oosman/Qt/5.5/clang_64/lib/cmake/Qt5Core
# CMAKE_OSX_DEPLOYMENT_TARGET 10.10
# CMAKE_OSX_ARCHITECTURES x86_64
# CMAKE_BUILD_TYPE Release
# press c to configure and g to generate, do not run cmake after you exit ccmake!ß
#make
# then for mac, run ../ScreenRecorder/install-scripts/macinstall.sh

cmake_minimum_required(VERSION 2.8)
SET(CMAKE_AUTOMOC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)
SET(CMAKE_C_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS} -Wall -Wextra" )
SET(CMAKE_CXX_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS} -Wall -Wextra" )
set(CMAKE_C_FLAGS_DEBUG "-g -DMYDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -DMYDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s -O3")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s -O3")

#rm -fr ../ScreenRecorder-build/*
#rm -fr ../ScreenRecorder-build/CMakeCache.txt

SET(PROJECT_NAME ScreenRecorder)
PROJECT(${PROJECT_NAME})

SET(HOME_DIR "/Users/oosman")
#SET(HOME_DIR "/Users/OsleTek")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(_MACHINE_ARCH_FLAG "/machine:x86")
    ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
#apt-file search Qt5MultimediaConfig.cmake
#sudo apt install qtmultimedia5-dev
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

SET(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
#add_compile_options(-std=c++11)
#add_compile_options(-stdlib=libstdc++)
#add_compile_options(-stdlib=libc++)

FIND_PACKAGE(Qt5Core REQUIRED)
FIND_PACKAGE(Qt5Concurrent REQUIRED)
FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Svg REQUIRED)
FIND_PACKAGE(Qt5Network REQUIRED)
FIND_PACKAGE(Qt5Multimedia REQUIRED)
FIND_PACKAGE(Qt5MultimediaWidgets REQUIRED)

#FIND_PACKAGE(Qt5WinExtras REQUIRED)
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
FIND_PACKAGE(Qt5X11Extras REQUIRED)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

INCLUDE_DIRECTORIES(
    ${QT_INCLUDES}
    ${Qt5Widgets_INCLUDES}
    ../.libs/zlib-1.2.8
    ../.libs/libpng-1.6.21
    ../.libs/jpeg-9
    ../.libs/openssl/include
    ../.libs/alsa-lib-1.1.4.1/include
    ../.libs/giflib/lib
    ../.libs/ffmpeg
    ../License
    ../Updater
    Gif
    .
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

ADD_DEFINITIONS(
    ${Qt5Widgets_DEFINITIONS}
    ${Qt5Concurrent_DEFINITIONS}
)

# this will run uic on .ui files:
QT5_WRAP_UI(PROJECT_UI_HDRS
    MainWindow.ui
    CameraWidget.ui
    AboutWidget.ui
    Settings.ui
    FFMpegRunnerDialog.ui
    TransparentMaximizedWindow.ui
    TabDialog.ui
    CrossCursorWidget.ui
    ../Updater/UpdaterWidget.ui
    ../License/LicenseWidget.ui
)
QT5_ADD_RESOURCES(PROJECT_RESOURCES
    resources.qrc
)
ADD_DEFINITIONS(
#    -DHAVE_LIBWEBP
    -DHAVE_STDINT_H
    -DHAVE_LIBPNG
    -DHAVE_LIBJPEG
    -DHAVE_LIBZ
    -DFT2_BUILD_LIBRARY
    -DFT_CONFIG_OPTION_SUBPIXEL_RENDERING
    -DNONDLL
)

SET(SoundLibs "")
SET(SSLLibs "")
SET(X11Libs "")
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    LINK_DIRECTORIES(
        ${CMAKE_CURRENT_SOURCE_DIR}/../libs/build/linux/bin/
        ${Qt5X11Extras_DIR}/../../../lib/
	/home/dev/.libs/ffmpeg/libavcodec/    
	/home/dev/.libs/ffmpeg/libavfilter/   
	/home/dev/.libs/ffmpeg/libavresample/ 
	/home/dev/.libs/ffmpeg/libpostproc/   
	/home/dev/.libs/ffmpeg/libswscale/    
	/home/dev/.libs/ffmpeg/libavdevice/   
	/home/dev/.libs/ffmpeg/libavformat/   
	/home/dev/.libs/ffmpeg/libavutil/     
	/home/dev/.libs/ffmpeg/libswresample
    )
    SET(SSLLibs
        crypto
        ssl
        dl
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
    SET(SoundLibs
        asound #/usr/lib/x86_64-linux-gnu/alsa-lib/libasound_module_conf_pulse.so
    )
    SET(X11Libs
        X11
        Xfixes #/usr/lib/x86_64-linux-gnu/libXfixes.so.3.1.0
        Qt5X11Extras #ln -s /home/developer/oosman/Qt5.12.1/5.12.1/gcc_64/lib/libQt5X11Extras.so ~/work/libs/build/linux/bin/libQt5X11Extras.so
    )
#ln -s /usr/lib/x86_64-linux-gnu/libpulse-mainloop-glib.so.0.0.5
    SET(JPEGLib
        jpeg
        )
    SET(GifLib
        gif
        )
    SET(PROJECT_LINUX_SOURCES
        AlsaIn.cpp
        AlsaIn.h
        X11Mouse.cpp
        X11Mouse.h
        X11Key.cpp
        X11Key.h
        X11WindowEnumerator.h
        X11WindowEnumerator.cpp
    )
    ADD_DEFINITIONS(
        -DLinux
    )
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    LINK_DIRECTORIES(
        ${HOME_DIR}/work/libs/build/osx/bin
    )
    SET(SSLLibs
        crypto
        ssl
        dl
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
    SET(ZLib
            z
        )
    SET(JPEGLib
        jpeg-9
        )
    SET(GifLib
        gif-7
        )
    SET(PNGLib
        png16
        )
    SET(MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT})
    SET(MACOSX_BUNDLE_ICON_FILE ${PROJECT_NAME}.ico)
    SET(App_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/${PROJECT_NAME}.ico)
    SET_SOURCE_FILES_PROPERTIES(${App_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    ADD_DEFINITIONS(
        -DDarwin
    )

ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    ADD_DEFINITIONS(
        -DWin64
    )
    LINK_DIRECTORIES(
        ${CMAKE_CURRENT_SOURCE_DIR}/../libs/build/win64
    )
    SET(SSLLibs
        ssl-1_1-x64
        crypto-1_1-x64
    )
     SET(FFMpegLibs
        avcodec-58
        avdevice-58
        avfilter-7
        avformat-58
        avutil-56
        swresample-3
        swscale-5
    )
    SET(ZLib
            z
        )
    SET(JPEGLib
        jpeg-9
        )
    SET(GifLib
        gif-7
        )
    SET(PNGLib
        png16-16
        )
    SET(SoundLibs
        Winmm
        )
    SET(PROJECT_WINDOWS_SOURCES
        WindowsMouse.h
        WindowsMouse.cpp
        WindowsKey.h
        WindowsKey.cpp
        WAV.cpp
        WAVEIN.cpp
        WindowsWindowEnumerator.h
        WindowsWindowEnumerator.cpp
        )
    SET(PLATFORM WIN32)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

SET(PROJECT_SOURCES
    cmdutils.c
    cmdutils.h
    ffwrite.h
    ffwrite.c
    main.cpp
    CameraWidget.h
    CameraWidget.cpp
    CrossCursorWidget.h
    CrossCursorWidget.cpp
    MouseInterface.h
    MouseInterface.cpp
    KeyInterface.h
    KeyInterface.cpp
    AudioInInterface.h
    AudioInInterface.cpp
    MainWindow.h
    MainWindow.cpp
    TransparentMaximizedWindow.h
    TransparentMaximizedWindow.cpp
    AboutWidget.h
    AboutWidget.cpp
    NullAudioIn.h
    NullAudioIn.cpp
    InterfaceFactory.h
    InterfaceFactory.cpp
    FFMpegRunner.h
    FFMpegRunner.cpp
    FFMpegRunnerDialog.h
    FFMpegRunnerDialog.cpp
    AudioFrameProducer.h
    AudioFrameProducer.cpp
    VideoFrameProducer.h
    VideoFrameProducer.cpp
    WindowEnumeratorInterface.h
    Settings.cpp
    TabDialog.h
    TabDialog.cpp
    Gif/GifMaker.h
    Gif/GifMaker.cpp
    Gif/ColorMap.h
    Gif/ColorMap.cpp
    Gif/Bucket.h
    Gif/Bucket.cpp
    WatermarkOpacityCalculator.h
    WatermarkOpacityCalculator.cpp
    ../Updater/UpdaterWidget.h
    ../Updater/UpdaterWidget.cpp
    ../License/NetworkRequest.h
    ../License/NetworkRequest.cpp
    ../License/License.h
    ../License/License.cpp
    ../License/LicenseComm.h
    ../License/LicenseComm.cpp
    ../License/LicenseWidget.h
    ../License/LicenseWidget.cpp
    resources/screenrecorder.rc
    ${PROJECT_WINDOWS_SOURCES}
    ${PROJECT_LINUX_SOURCES}
)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET(CMAKE_INSTALL_RPATH "/usr/local/bin/bf-booby-screen-recorder")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")


ADD_EXECUTABLE(${PROJECT_NAME} WIN32 MACOSX_BUNDLE
    ${App_ICON}
    ${PROJECT_SOURCES}
    ${PROJECT_MOC_SRCS}
    ${PROJECT_UI_HDRS}
    ${PROJECT_RESOURCES}
)
#MESSAGE(STATUS "Baah Hum Bug!")

TARGET_LINK_LIBRARIES(${PROJECT_NAME}
    ${Qt5Widgets_LIBRARIES}
    Qt5::Network
    Qt5::Multimedia
    Qt5::MultimediaWidgets
    ${SSLLibs}
    ${ZLib}
    ${GifLib}
    ${JPEGLib}
    ${PNGLib}
    ${FFMpegLibs}
    ${SoundLibs}
    ${X11Libs}
)
IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS -LC:/work/libs/build/win64)
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS -Wl,-subsystem,windows)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS -L/usr/local/bin/bf-booby-screen-recorder/)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES MACOSX_BUNDLE TRUE)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

