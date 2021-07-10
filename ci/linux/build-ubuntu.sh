#!/bin/sh
set -ex

cp LICENSE data/LICENSE-$PLUGIN_NAME

sed -i 's;${CMAKE_INSTALL_FULL_LIBDIR};/usr/lib;' CMakeLists.txt
grep LIBVNCSERVER_HAVE_SASL /usr/include/rfb/rfbconfig.h || true

mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make -j4
