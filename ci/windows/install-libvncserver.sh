#! /bin/bash

set -ex

if test ! -d $LIBVNCPath; then
	echo obs-studio directory does not exist
	git clone https://github.com/LibVNC/libvncserver.git $LIBVNCPath
fi

cd $LIBVNCPath
OBSLatestTag=$(git describe --tags --exclude="*-rc*" --abbrev=0)
git checkout $OBSLatestTag

# below are copied from libvncserver/.appveyor.yml
mkdir -p $LIBVNCPath/deps
cd $LIBVNCPath/deps

case $CMakeOptA in
	Win32)
		curl -fsSL -o nasm.zip https://www.nasm.us/pub/nasm/releasebuilds/2.15.05/win32/nasm-2.15.05-win32.zip
		;;
	x64)
		curl -fsSL -o nasm.zip https://www.nasm.us/pub/nasm/releasebuilds/2.15.05/win64/nasm-2.15.05-win64.zip
		;;
esac
7z x nasm.zip
mv nasm-2.15.05 nasm
PATH="$PATH:$PWD/nasm"

curl -fsSL -o lzo.tar.gz http://www.oberhumer.com/opensource/lzo/download/lzo-2.10.tar.gz
mkdir -p lzo/build
cd lzo
tar xzf ../lzo.tar.gz --strip-components=1
cd build
cmake -A $CMakeOptA ..
cmake --build . --config $build_config
cd $LIBVNCPath/deps

# curl -fsSL -o libjpeg.tar.gz https://github.com/libjpeg-turbo/libjpeg-turbo/archive/2.0.4.tar.gz
curl -fsSL -o libjpeg.tar.gz https://github.com/norihiro/obs-vnc/releases/download/0.1.0/libjpeg-2.0.4-norihiro.tar.gz
mkdir -p libjpeg
cd libjpeg
tar xzf ../libjpeg.tar.gz --strip-components=1
cmake -A $CMakeOptA .
cmake --build . --config $build_config
cd $LIBVNCPath/deps

curl -fsSL -o libpng.tar.gz https://downloads.sourceforge.net/project/libpng/libpng16/1.6.40/libpng-1.6.40.tar.gz
mkdir -p libpng
cd libpng
tar xzf ../libpng.tar.gz --strip-components=1
cmake . -A $CMakeOptA -DZLIB_INCLUDE_DIR=$OBSDeps/include -DZLIB_LIBRARY=$OBSDeps/lib/zlib.lib
cmake --build . --config $build_config
cd ..

# REM Berkeley DB - required by SASL
# curl -fsSL -o db-4.1.25.tar.gz http://download.oracle.com/berkeley-db/db-4.1.25.tar.gz
# 7z x db-4.1.25.tar.gz -so | 7z x -si -ttar > /dev/null
# mv db-4.1.25 db
# cd db\build_win32
# echo using devenv %DEVENV_EXE%
# %DEVENV_EXE% db_dll.dsp /upgrade
# cmake --build . --config %build_config%
# cd ..\..

# REM Cyrus SASL
# curl -fsSL -o cyrus-sasl-2.1.27.tar.gz https://github.com/cyrusimap/cyrus-sasl/releases/download/cyrus-sasl-2.1.27/cyrus-sasl-2.1.27.tar.gz
# 7z x cyrus-sasl-2.1.27.tar.gz -so | 7z x -si -ttar > /dev/null
# mv cyrus-sasl-2.1.27 sasl
# cd sasl
# patch -p1 -i ..\sasl-fix-snprintf-macro.patch
# echo using vsdevcmd %VSDEVCMD_BAT%
# '%VSDEVCMD_BAT%'
# nmake /f NTMakefile OPENSSL_INCLUDE=c:\OpenSSL-Win32\include OPENSSL_LIBPATH=c:\OpenSSL-Win32\lib DB_INCLUDE=c:\projects\libvncserver\deps\db\build_win32 DB_LIBPATH=c:\projects\libvncserver\deps\db\build_win32\release DB_LIB=libdb41.lib install
# cd ..

# go back to source root
cd ..

echo Preparing and building libvncserver...
cmake \
	-A $CMakeOptA \
	-DWITH_OPENSSL=OFF \
	-DZLIB_INCLUDE_DIR=$OBSDeps/include -DZLIB_LIBRARY=$OBSDeps/lib/zlib.lib \
	-DLZO_INCLUDE_DIR=$LIBVNCPath/deps/lzo/include -DLZO_LIBRARIES=$LIBVNCPath/deps/lzo/build/$build_config/lzo2.lib \
	-DPNG_PNG_INCLUDE_DIR=$LIBVNCPath/deps/libpng \
	-DPNG_INCLUDE_DIR=$LIBVNCPath/deps/libpng \
	-DPNG_LIBRARY=$LIBVNCPath/deps/libpng/$build_config/libpng16_static.lib \
	-DJPEG_INCLUDE_DIR=$LIBVNCPath/deps/libjpeg -DJPEG_LIBRARY=./deps/libjpeg/$build_config/turbojpeg-static \
	-DWITH_SDL=OFF -DWITH_GTK=OFF -DWITH_SYSTEMD=OFF -DWITH_FFMPEG=OFF \
	-D LIBVNCSERVER_INSTALL=OFF \
	-D WITH_TIGHTVNC_FILETRANSFER=OFF \
	-D WITH_EXAMPLES=OFF \
	.
cmake --build . --config $build_config
