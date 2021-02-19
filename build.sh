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
	CMAKE_PREFIX_PATH=/home/dev/oosman/Qt/5/5.15.2/gcc_64/lib/cmake/ CMAKE_BUILD_TYPE=Debug cmake ..
	make -j$(getconf _NPROCESSORS_ONLN)
popd
