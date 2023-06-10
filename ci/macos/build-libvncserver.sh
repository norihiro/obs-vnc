#! /bin/bash

set -ex

brew install pkg-config

mkdir deps

curl -L -o deps/lzo.tar.gz http://www.oberhumer.com/opensource/lzo/download/lzo-2.10.tar.gz
mkdir deps/lzo/
tar -xzf deps/lzo.tar.gz -C deps/lzo/ --strip-components 1

pushd deps/lzo
cmake '-DCMAKE_OSX_ARCHITECTURES=x86_64;arm64' -B build
cmake --build build
sudo cmake --install build
popd
file deps/lzo/build/liblzo2.a
cat deps/lzo/build/lzo2.pc

curl -L -o deps/libjpeg.tar.gz https://github.com/norihiro/obs-vnc/releases/download/0.1.0/libjpeg-2.0.4-norihiro.tar.gz
mkdir deps/libjpeg
tar -xzf deps/libjpeg.tar.gz -C deps/libjpeg --strip-components 1

pushd deps/libjpeg
cmake '-DCMAKE_OSX_ARCHITECTURES=x86_64;arm64' -B build -DENABLE_SHARED=OFF
cmake --build build
sudo cmake --install build
popd
file deps/libjpeg/build/lib*jpeg.a

curl -L -o deps/libpng.tar.gz https://downloads.sourceforge.net/project/libpng/libpng16/1.6.39/libpng-1.6.39.tar.gz
mkdir deps/libpng
tar -xzf deps/libpng.tar.gz -C deps/libpng --strip-components 1

pushd deps/libpng
# TODO: apply arm-neon patch
cmake '-DCMAKE_OSX_ARCHITECTURES=x86_64;arm64' -B build \
	-DPNG_TESTS=OFF -DPNG_STATIC=ON -DPNG_SHARED=OFF -DPNG_DEBUG=OFF \
	-DCMAKE_ASM_FLAGS='-DPNG_ARM_NEON_IMPLEMENTATION=1' -DPNG_ARM_NEON=on
mkdir -p build/arm64
cmake --build build
sudo cmake --install build
popd
file deps/libpng/build/libpng16.a

##############################################################################################################

curl -L -o deps/gnutls.tar.xz https://www.gnupg.org/ftp/gcrypt/gnutls/v3.6/gnutls-3.6.16.tar.xz
mkdir deps/gnutls
tar xJf deps/gnutls.tar.xz -C deps/gnutls --strip-components 1

for a in arm64 x86_64; do
	mkdir deps/gnutls/build-$a
	pushd deps/gnutls/build-$a
	case "$a" in
		arm64)
			config_opts=(
				--host aarch64-apple-darwin20
				--prefix=/tmp/deps/arm64
				CFLAGS=-I/tmp/deps/arm64/include
				LDFLAGS=-L/tmp/deps/arm64/lib
			)
			;;
		x86_64)
			config_opts=()
			;;
	esac
	PKG_CONFIG_PATH=/tmp/deps/$a/lib/pkgconfig: \
		../configure "${config_opts[@]}" \
		--disable-shared \
		--enable-static
	make -j4
	popd
done


##############################################################################################################

git clone https://github.com/LibVNC/libvncserver.git deps/libvncserver
pushd deps/libvncserver
git checkout $(git describe --tags --exclude="*-rc*" --abbrev=0)
cmake \
	'-DCMAKE_OSX_ARCHITECTURES=x86_64;arm64' \
	-DWITH_OPENSSL=ON \
	-DWITH_SDL=OFF -DWITH_GTK=OFF -DWITH_SYSTEMD=OFF -DWITH_FFMPEG=OFF \
	-DLIBVNCSERVER_INSTALL=OFF \
	-DWITH_TIGHTVNC_FILETRANSFER=OFF \
	-DWITH_EXAMPLES=OFF \
	-B build
cmake --build . --config $build_config

test -f "$GITHUB_OUTPUT" && echo 'success=true' >> $GITHUB_OUTPUT
