#to build on MacOS/Ubuntu from command line
#mkdir ../ScreenRecorder-build
#./ccmake ../ScreenRecorder
# or try
#CMAKE_INCLUDE_PATH=/home/dev/osaeed/Qt/5.15.0/Src/qtwebengine/src/3rdparty/chromium/third_party/mesa_headers/ ccmake ..
# c to configure, get some errors, copy name of missing Qt package, exit, find ~/ -name Qt5CoreConfig.cmake, copy path, retry
# after compiling code, to run application, in ScreenRecorder-build:
#ln -s ${HOME}/${DOCKERUSER}/Qt5.152.0/Tools/QtCreator/lib/Qt/plugins/platforms platforms
#
#LINUX
# Qt5Concurrent_DIR                /home/dev/oosman/Qt/5.15.0/gcc_64/lib/cmake/Qt5Concurrent        
# Qt5Core_DIR                      /home/dev/oosman/Qt/5.15.0/gcc_64/lib/cmake/Qt5Core
# Qt5Gui_DIR                       /home/dev/oosman/Qt/5.15.0/gcc_64/lib/cmake/Qt5Gui
# Qt5MultimediaWidgets_DIR         /home/dev/oosman/Qt/5.15.0/gcc_64/lib/cmake/Qt5MultimediaWidgets 
# Qt5Multimedia_DIR                /home/dev/oosman/Qt/5.15.0/gcc_64/lib/cmake/Qt5Multimedia        
# Qt5Network_DIR                   /home/dev/oosman/Qt/5.15.0/gcc_64/lib/cmake/Qt5Network           
# Qt5Svg_DIR                       /home/dev/oosman/Qt/5.15.0/gcc_64/lib/cmake/Qt5Svg
# Qt5Widgets_DIR                   /home/dev/oosman/Qt/5.15.0/gcc_64/lib/cmake/Qt5Widgets           
# Qt5X11Extras_DIR                 /home/dev/oosman/Qt/5.15.0/gcc_64/lib/cmake/Qt5X11Extras# CMAKE_OSX_SYSROOT 
#/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.10.sdk
#( you can get SDKs from https://github.com/phracker/MacOSX-SDKs/releases )
# Qt5Core_DIR /Users/oosman/Qt/5.5/clang_64/lib/cmake/Qt5Core
# CMAKE_OSX_DEPLOYMENT_TARGET 10.10
# CMAKE_OSX_ARCHITECTURES x86_64
# CMAKE_BUILD_TYPE Release
# press c to configure and g to generate, do not run cmake after you exit ccmake!ß
#make
# then for mac, run ../ScreenRecorder/install-scripts/macinstall.sh

cmake_minimum_required(VERSION 3.19)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_C_FLAGS  -Wall)
set(CMAKE_CXX_FLAGS  -Wall)
#set(CMAKE_C_FLAGS_DEBUG -g -DMYDEBUG)
#set(CMAKE_CXX_FLAGS_DEBUG -g -DMYDEBUG)
set(CMAKE_C_FLAGS_RELEASE  -s -O3)
set(CMAKE_CXX_FLAGS_RELEASE  -s -O3)
set(LIBS_DIR "$ENV{HOME}/$ENV{DOCKERUSER}/.leila/lib")
set(QT_DIR "$ENV{HOME}/$ENV{DOCKERUSER}/Qt/5.15.0")

#rm -fr ../ScreenRecorder-build/*
#rm -fr ../ScreenRecorder-build/CMakeCache.txt

SET(PROJECT_NAME ScreenRecorder)
PROJECT(${PROJECT_NAME})

SET(HOME_DIR /Users/oosman)
#SET(HOME_DIR /Users/OsleTek)

IF( MATCHES Windows)
    set(_MACHINE_ARCH_FLAG /machine:x86)
    ENDIF( MATCHES Windows)
IF(UNIX)
#apt-file search Qt5MultimediaConfig.cmake
#
ENDIF(UNIX)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_FLAGS  -std=c++17)
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

IF(UNIX)
FIND_PACKAGE(Qt5X11Extras REQUIRED)
#sudo apt-get install -y libqt5x11extras5-dev
ELSEIF(WIN32)
FIND_PACKAGE(Qt5WinExtras REQUIRED)
ENDIF(UNIX)

INCLUDE_DIRECTORIES(
    ${LIBS_DIR}/zlib-1.2.11
    ${LIBS_DIR}/libpng-1.6.37
    ${LIBS_DIR}/turbojpeg
#    ${LIBS_DIR}/jpegsr9c
    ${LIBS_DIR}/openssl/include
    ${LIBS_DIR}/alsa/include
    ${LIBS_DIR}/libgif/lib
    ${LIBS_DIR}/ffmpeg
    ../License
    ../Updater
    Gif
    .
)

ADD_DEFINITIONS(
    
    
)

# this will run uic on .ui files:
QT5_WRAP_UI(PROJECT_UI_HDRS
    ../License/LicenseWidget.ui
    ../Updater/UpdaterWidget.ui
    AboutWidget.ui
    CameraWidget.ui
    CrossCursorWidget.ui
    FFMpegRunnerDialog.ui
    MainWindow.ui
    Playback/PreviewDialog.ui
    Settings.ui
    TabDialog.ui
    TransparentMaximizedWindow.ui
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

SET(SoundLibs )
SET(SSLLibs )
SET(X11Libs )
IF(UNIX)
    LINK_DIRECTORIES(
        ${LIBS_DIR}/ffmpeg/libavcodec/
        ${LIBS_DIR}/ffmpeg/libavfilter/
        ${LIBS_DIR}/ffmpeg/libavresample/
        ${LIBS_DIR}/ffmpeg/libpostproc/
        ${LIBS_DIR}/ffmpeg/libswscale/
        ${LIBS_DIR}/ffmpeg/libavdevice/
        ${LIBS_DIR}/ffmpeg/libavformat/
        ${LIBS_DIR}/ffmpeg/libavutil/
        ${LIBS_DIR}/ffmpeg/libswresample/
        ${LIBS_DIR}/libgif/lib/.libs/
        ${LIBS_DIR}/turbojpeg/build/
        ${LIBS_DIR}/alsa/src/.libs/
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
        turbojpeg
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
ENDIF(UNIX)

IF( MATCHES Darwin)
    LINK_DIRECTORIES(
        /work/libs/build/osx/bin
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
    SET(MACOSX_BUNDLE_BUNDLE_NAME )
    SET(MACOSX_BUNDLE_ICON_FILE .ico)
    SET(App_ICON /resources/.ico)
    SET_SOURCE_FILES_PROPERTIES( PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    ADD_DEFINITIONS(
        -DDarwin
    )

ENDIF( MATCHES Darwin)

IF( MATCHES Windows)
    ADD_DEFINITIONS(
        -DWin64
    )
    LINK_DIRECTORIES(
        /../libs/build/win64
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
ENDIF( MATCHES Windows)

SET(PROJECT_SOURCES
    ${PROJECT_LINUX_SOURCES}
    ${PROJECT_WINDOWS_SOURCES}
    ../License/License.cpp
    ../License/License.h
    ../License/LicenseComm.cpp
    ../License/LicenseComm.h
    ../License/LicenseWidget.cpp
    ../License/LicenseWidget.h
    ../License/NetworkRequest.cpp
    ../License/NetworkRequest.h
    ../Updater/UpdaterWidget.cpp
    ../Updater/UpdaterWidget.h
    AboutWidget.cpp
    AboutWidget.h
    AudioFrameProducer.cpp
    AudioFrameProducer.h
    AudioInInterface.cpp
    AudioInInterface.h
    BufferWriter.cpp
    BufferWriter.h
    CameraWidget.cpp
    CameraWidget.h
    CrossCursorWidget.cpp
    CrossCursorWidget.h
    FFMpegRunner.cpp
    FFMpegRunner.h
    FFMpegRunnerDialog.cpp
    FFMpegRunnerDialog.h
    Gif/Bucket.cpp
    Gif/Bucket.h
    Gif/ColorMap.cpp
    Gif/ColorMap.h
    Gif/GifMaker.cpp
    Gif/GifMaker.h
    InterfaceFactory.cpp
    InterfaceFactory.h
    KeyInterface.cpp
    KeyInterface.h
    LinuxFileWriter.cpp
    LinuxFileWriter.h
    MainWindow.cpp
    MainWindow.h
    MouseInterface.cpp
    MouseInterface.h
    NullAudioIn.cpp
    NullAudioIn.h
    Playback/ffplay3.c
    Playback/ffplay3.h
    Playback/PreviewDialog.cpp
    Playback/PreviewDialog.h
    Playback/ScreenStreamer.h
    Playback/ScreenStreamer.cpp
    Playback/SocketReader.cpp
    Playback/SocketReader.h
    Settings.cpp
    SocketWriter.cpp
    SocketWriter.h
    TabDialog.cpp
    TabDialog.h
    TransparentMaximizedWindow.cpp
    TransparentMaximizedWindow.h
    VideoFrameProducer.cpp
    VideoFrameProducer.h
    WatermarkOpacityCalculator.cpp
    WatermarkOpacityCalculator.h
    WindowEnumeratorInterface.h
    cmdutils.c
    cmdutils.h
    ffwrite.c
    ffwrite.h
    main.cpp
    resources/screenrecorder.rc
)

IF(UNIX)
    SET(CMAKE_INSTALL_RPATH /usr/local/bin/bf-booby-screen-recorder)
ENDIF(UNIX)

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
    pthread
)
IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS -LC:/work/libs/build/win64)
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS -Wl,-subsystem,windows)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS -L/usr/local/bin/bf-booby-screen-recorder/)
    execute_process(COMMAND bash "-c" "/bin/ln -sf ${QT_DIR}/gcc_64/plugins/platforms platforms"
    OUTPUT_VARIABLE LN_CMD_RES
    OUTPUT_STRIP_TRAILING_WHITESPACE
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES MACOSX_BUNDLE TRUE)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
