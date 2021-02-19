#!/bin/bash -xe
set -xe

pushd ..
	if [ ! -d "License" ]; then
		git clone https://github.com/jambamamba/License.git
	fi
	if [ ! -d "Updater" ]; then
		git clone https://github.com/jambamamba/Updater.git
	fi
	if [ ! -d "DS" ]; then
		git clone https://github.com/jambamamba/DS.git
	fi
popd


mkdir -p build
pushd build
	rm -fr *
	CMAKE_PREFIX_PATH=$HOME/$DOCKERUSER/Qt/5/5.15.2/gcc_64/lib/cmake/ cmake -DCMAKE_BUILD_TYPE=Debug	  ..
	make -j$(getconf _NPROCESSORS_ONLN)
popd
