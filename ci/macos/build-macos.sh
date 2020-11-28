#!/bin/sh

OSTYPE=$(uname)

if [ "${OSTYPE}" != "Darwin" ]; then
    echo "[Error] macOS build script can be run on Darwin-type OS only."
    exit 1
fi

HAS_CMAKE=$(type cmake 2>/dev/null)

if [ "${HAS_CMAKE}" = "" ]; then
    echo "[Error] CMake not installed - please run 'install-dependencies-macos.sh' first."
    exit 1
fi

#export QT_PREFIX="$(find /usr/local/Cellar/qt5 -d 1 | tail -n 1)"

# workaround that -DLIBVNCCLIENT_INCLUDE_DIRS cannot accept two or more paths
cp -a ../libvncserver/build/rfb/rfbconfig.h ../libvncserver/rfb/

echo "=> Building plugin for macOS."
mkdir -p build && cd build
cmake .. \
	-DQTDIR=/tmp/obsdeps \
	-DLIBOBS_INCLUDE_DIR=../../obs-studio/libobs \
	-DLIBOBS_LIB=../../obs-studio/libobs \
	-DOBS_FRONTEND_LIB="$(pwd)/../../obs-studio/build/UI/obs-frontend-api/libobs-frontend-api.dylib" \
	-DLIBVNCCLIENT_INCLUDE_DIRS="$(pwd)/../../libvncserver" \
	-DLIBVNCCLIENT_LIB_DIRS="$(pwd)/../../libvncserver/build" \
	-DLIBVNCCLIENT_LIBRARIES="$(pwd)/../../libvncserver/build/libvncclient.a;/usr/local/lib/liblzo2.dylib;/usr/local/lib/libjpeg.dylib;/usr/local/lib/libpng.dylib;libsasl2.tbd;libz.tbd" \
	-DCMAKE_BUILD_TYPE=RelWithDebInfo \
	-DCMAKE_INSTALL_PREFIX=/usr \
&& make -j4 VERBOSE=1
