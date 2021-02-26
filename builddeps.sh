#!/bin/bash -xe
set -xe

source /home/dev/.scripts/utils.sh

function installZLib()
{
	libExists LIB="libz.so" RESULT=0
	if [ $RESULT -gt 0 ]; then return 0; fi

	local LIB="zlib-1.2.11"
	local EXT="tar.gz"
	local URL="https://www.zlib.net/${LIB}.${EXT}"
	procureLib LIB=${LIB} EXT=${EXT} URL=${URL}
	makeLib LIB=${LIB} CONF_FLAGS="--enable-shared"
}

function installPngLib()
{
	local LIB="libpng-1.6.37"
	libExists LIB="${LIB}" RESULT=0
	if [ $RESULT -gt 0 ]; then return 0; fi

	local EXT="tar.xz"
	local URL="https://download.sourceforge.net/libpng/${LIB}.${EXT}"
	procureLib LIB=${LIB} EXT=${EXT} URL=${URL}
	makeLib LIB=${LIB} CONF_FLAGS="--enable-shared"
}

function prepareJpegSources()
{
	parseArgs $@

	pushd $LIBS_DIR/${LIB}
	sed -i -e 's/\r//g' configure #remove ^M character
	sed -i -e 's/\r//g' config.sub #remove ^M character
	sed -i -e 's/\r//g' config.guess #remove ^M character
	chmod +x configure
	autoreconf -if
	popd
}

function installJpegLib()
{
	libExists LIB="jpeg" RESULT=0
	if [ $RESULT -gt 0 ]; then return 0; fi

	local LIB="jpegsr9c"
	local EXT="zip"
	local URL="http://www.ijg.org/files/${LIB}.${EXT}"
	procureLib LIB=${LIB} EXT=${EXT} URL=${URL}
	prepareJpegSources LIB=${LIB} EXT=${EXT} URL=${URL}
	makeLib LIB=${LIB} CONF_FLAGS="--enable-shared"

	find $LIBS_DIR/${LIB}/ -name "*.h" | xargs sudo cp -Pt /usr/local/include/
}

function installTurboJpegLib()
{
	libExists LIB="turbojpeg" RESULT=0
	if [ $RESULT -gt 0 ]; then return 0; fi

	local LIB="turbojpeg"
	procureLib SCM="git" SCM_CMD="clone" URL="https://github.com/libjpeg-turbo/libjpeg-turbo.git" LIB=$LIB
	makeLib LIB=${LIB} BUILDSYSTEM="cmake"

	#find $LIBS_DIR/${LIB}/ -name "*.h" | xargs sudo cp -Pt /usr/local/include/
}

function installFreetypeLib()
{
	local LIB="freetype-2.10.4"
	libExists LIB="$LIB" RESULT=0
	if [ $RESULT -gt 0 ]; then return 0; fi

	local EXT="tar.gz"
	local URL="https://download.savannah.gnu.org/releases/freetype/${LIB}.${EXT}"
	procureLib LIB=${LIB} EXT=${EXT} URL=${URL}
	makeLib LIB=${LIB} CONF_FLAGS="--enable-shared"
}

function installXiphLibrary()
{
	parseArgs $@

	local LIB_NAME=$LIB
	libExists LIB="lib${LIB}" RESULT=0
	if [ $RESULT -gt 0 ]; then return 0; fi

	procureLib SCM="git" SCM_CMD="clone" URL="https://github.com/xiph/${LIB_NAME}.git" LIB=${LIB}
	pushd $LIBS_DIR/${LIB}
	./autogen.sh
	popd
	makeLib LIB=${LIB} CONF_FLAGS="--enable-shared"
	pushd $LIBS_DIR/${LIB}
	sudo make install
	popd
}

function installMp3Lame()
{
	local LIB="libmp3lame"
	libExists LIB="$LIB" RESULT=0
	if [ $RESULT -gt 0 ]; then return 0; fi

	procureLib SCM="svn" SCM_CMD="checkout" URL="https://svn.code.sf.net/p/lame/svn/trunk/lame" LIB=$LIB
	makeLib LIB=${LIB} CONF_FLAGS="--enable-shared#space#--enable-nasm"
}

function installVpx()
{
	local LIB="libvpx"
	libExists LIB="$LIB" RESULT=0
	if [ $RESULT -gt 0 ]; then return 0; fi

	procureLib SCM="git" SCM_CMD="clone" URL="https://chromium.googlesource.com/webm/${LIB}.git" LIB=$LIB
	makeLib LIB=${LIB} CFLAGS="-fPIC" CONF_FLAGS="--enable-shared#space#--enable-vp8#space#--enable-vp9#space#--enable-webm-io"
}

function installX264()
{
	local LIB="libx264"
	libExists LIB="$LIB" RESULT=0
	if [ $RESULT -gt 0 ]; then return 0; fi

	procureLib SCM="git" SCM_CMD="clone" URL="https://code.videolan.org/videolan/x264.git" LIB=$LIB
	makeLib LIB=${LIB} CONF_FLAGS="--enable-shared#space#--disable-asm"
	pushd $BUILD_DIR
	ln -s libx264.so.161 libx264.so
	popd
}

function installFFMpegDependencies()
{
	installMp3Lame
	installXiphLibrary LIB=ogg
	installXiphLibrary LIB=vorbis
	installXiphLibrary LIB=theora
	installVpx
	installX264
}

function copyFFMpegBinaries()
{
	cp -f "$LIBS_DIR/ffmpeg/ffmpeg" $DOCKER_BIN/
	cp -f "$LIBS_DIR/ffmpeg/ffplay" $DOCKER_BIN/
}

function installFFMpeg()
{
       installFFMpegDependencies

       libExists LIB="avcodec" RESULT=0
       if [ $RESULT -gt 0 ]; then
               copyFFMpegBinaries
               return 0;
       fi
	
	local LIB="ffmpeg"
	procureLib SCM="git" SCM_CMD="clone" URL="https://git.ffmpeg.org/$LIB.git" LIB=$LIB

	export PKG_CONFIG_PATH=$LIBS_DIR/../pkg_config
	mkdir -p $PKG_CONFIG_PATH
	cp -f $LIBS_DIR/libvorbis/vorbisenc.pc $PKG_CONFIG_PATH
	cp -f $LIBS_DIR/libvorbis/vorbis.pc $PKG_CONFIG_PATH
        cp -f $LIBS_DIR/libogg/ogg.pc $PKG_CONFIG_PATH
        cp -f $LIBS_DIR/libvpx/vpx.pc $PKG_CONFIG_PATH

	local INC_DIR=$LIBS_DIR/../include
	mkdir -p $INC_DIR
        mkdir -p $INC_DIR/lame
        cp -fr $LIBS_DIR/libmp3lame/include/lame.h $INC_DIR/lame
        cp -fr $LIBS_DIR/libtheora/include/* $INC_DIR
        cp -fr $LIBS_DIR/libogg/include/* $INC_DIR
        cp -fr $LIBS_DIR/libvpx/vpx $INC_DIR
	cp -fr $LIBS_DIR/libvorbis/include/* $INC_DIR
	cp -fr $LIBS_DIR/libx264/x264.h $INC_DIR
	cp -fr $LIBS_DIR/libx264/x264_config.h $INC_DIR

	makeLib LIB=${LIB} CONF_FLAGS="--enable-shared#space#--arch#equal#x86#space#--enable-libvpx#space#--enable-libtheora#space#--disable-encoder#equal#vorbis#space#--enable-libvorbis#space#--enable-libmp3lame#space#--enable-libx264#space#--enable-gpl#space#--enable-ffplay#space#--extra-cflags#equal#-I$INC_DIR/#space#--extra-ldflags#equal#-L$BUILD_DIR"
	
	copyFFMpegBinaries
}

function installOpenCV()
{
	local LIB="opencv"
	libExists LIB="$LIB" RESULT=0
	if [ $RESULT -gt 0 ]; then return 0; fi

	procureLib SCM="git" SCM_CMD="clone" URL="https://github.com/opencv/opencv.git" LIB=$LIB
	makeLib LIB=${LIB} CFLAGS="-DBUILD_TESTS=OFF" BUILDSYSTEM="cmake"
}

function installAlsa()
{
	libExists LIB="sound" RESULT=0
	if [ $RESULT -gt 0 ]; then return 0; fi

	local LIB="libalsa"
	procureLib SCM="git" SCM_CMD="clone" URL="https://github.com/alsa-project/alsa-lib.git" LIB=$LIB
	pushd $LIBS_DIR/${LIB}
	libtoolize
	aclocal
	automake --add-missing --force-missing --copy --foreign && true
	autoreconf
	popd
	makeLib LIB=${LIB} CONF_FLAGS="--enable-shared"
}

function installGifLib()
{
	local LIB="libgif"
	libExists LIB="$LIB" RESULT=0
	if [ $RESULT -gt 0 ]; then return 0; fi

	procureLib SCM="git" SCM_CMD="clone" URL="https://github.com/mldbai/giflib.git" LIB=$LIB
	makeLib LIB=${LIB} CONF_FLAGS="--enable-shared"
}


function installAllLibs()
{
	sudo chown -R dev:dev $BUILD_DIR
	sudo chown -R dev:dev $BUILD_DIR
	sudo chown -R dev:dev $BUILD_DIR

	echo "$BUILD_DIR" > /tmp/leila.conf
	sudo mv /tmp/leila.conf /etc/ld.so.conf.d/leila.conf
	sudo ldconfig
}

function main()
{
	configureLibsDirectory
        installZLib
        installPngLib
        #installJpegLib
	installTurboJpegLib
        installFreetypeLib
        installAlsa
        installGifLib
	installFFMpeg
        installOpenCV
	installAllLibs
}

configureLibsDirectory
installAllLibs
exit 0

main
