#! /bin/sh

set -ex

brew install pkg-config
removes=(lzo libpng jpeg-turbo)
for pkg in "${removes[@]}"; do
	brew uninstall --ignore-dependencies $pkg || true
done

b='http://www.nagater.net/obs-studio'
ff=(
	$b/0b22b3a294ae9a0adeaa234fa8439696e2484f2494a1fbad722cc5fd2dc2f68e--lzo--2.10.arm64_big_sur.bottle.tar.gz
	$b/2d56e746f5a3121427f2a75bfc39bc22c0ff418e73dee2fea3bad7a8106455a6--libpng--1.6.37.arm64_big_sur.bottle.tar.gz
	$b/d71bc8d6de400752604b81595ad7aa1339b00e0135b9c06b9a9abc90ddf5c9aa--jpeg-turbo--2.1.3.arm64_big_sur.bottle.tar.gz
)

dest='/usr/local/opt/arm64/'
sudo mkdir -p $dest
sudo chown $USER:staff $dest
cd $dest
for f in ${ff[@]}; do
	curl -O $f
	b=$(basename "$f")
	tar --strip-components=2 -xzf $b

	case "$b" in
		*lzo*)
			cp COPYING $OLDPWD/COPYING-lzo
			;;
		*libpng*)
			cp LICENSE $OLDPWD/LICENSE-libpng
			;;
		*jpeg-turbo*)
			cp share/doc/libjpeg-turbo/LICENSE.md $OLDPWD/LICENSE-jpeg-turbo
			;;
		*)
			echo 'Warning: Not copying license file.'
	esac
done

sha256sum -c <<-EOF
76d0933f626d8a1645b559b1709396a2a6fd57dbd556d2f1f1848b5fddfcd327  0b22b3a294ae9a0adeaa234fa8439696e2484f2494a1fbad722cc5fd2dc2f68e--lzo--2.10.arm64_big_sur.bottle.tar.gz
766a7136ee626b411fb63da0c7e5bc1e848afb6e224622f25ea305b2d1a4a0f1  2d56e746f5a3121427f2a75bfc39bc22c0ff418e73dee2fea3bad7a8106455a6--libpng--1.6.37.arm64_big_sur.bottle.tar.gz
e47a04f605090bfa71f2bbc71c84ea4f2a0c3986ccc8f61859b31247ed0e9e08  d71bc8d6de400752604b81595ad7aa1339b00e0135b9c06b9a9abc90ddf5c9aa--jpeg-turbo--2.1.3.arm64_big_sur.bottle.tar.gz
EOF

find $dest -name '*.dylib' |
while read dylib; do
	opt_inst=($(otool -L $dylib | awk -v dest="$dest" '$1~/^@@HOMEBREW_PREFIX@@/{
		chg0 = $1
		chg1 = chg0
		gsub(/^@@HOMEBREW_PREFIX@@\/opt\/[^\/]*\//, dest "/", chg1)
		print "-change", chg0, chg1
	}'))
	install_name_tool "${opt_inst[@]}" -id $dylib "$dylib"
done

cd -

cd ..

git clone https://github.com/LibVNC/libvncserver.git
cd libvncserver
OBSLatestTag=$(git describe --tags --abbrev=0)
git checkout $OBSLatestTag
mkdir build && cd build
echo "=> Building libvncserver.."
export PKG_CONFIG_PATH=$dest/lib/pkgconfig:
cmake .. \
	-DCMAKE_BUILD_TYPE=RelWithDebInfo \
	-DCMAKE_OSX_ARCHITECTURES=$arch \
	-DCMAKE_OSX_DEPLOYMENT_TARGET=${MACOSX_DEPLOYMENT_TARGET} \
	-DCMAKE_INSTALL_PREFIX="$deps" \
	-DWITH_OPENSSL=OFF -DWITH_GNUTLS=OFF -DWITH_GCRYPT=OFF \
	-DBUILD_SHARED_LIBS=OFF \
	-DWITH_SDL=OFF \
	-DWITH_GTK=OFF \
	-DWITH_SYSTEMD=OFF \
	-DWITH_FFMPEG=OFF \
	-DCMAKE_FRAMEWORK_PATH="$dest;$dest/lib;$dest/lib/cmake;$dest/lib/cmake/libjpeg-turbo/;$deps/Frameworks;$deps/lib/cmake;$deps" \
	-D LZO_LIBRARIES=$dest/lib/liblzo2.dylib \
	-D LZO_INCLUDE_DIR=$dest/include \
	-D JPEG_INCLUDE_DIR=$dest/include

make -j4
make install

patch $deps/lib/pkgconfig/libvncclient.pc <<-EOF
--- /tmp/deps-arm64/lib/pkgconfig/libvncclient.pc.org	2022-08-11 23:41:10.000000000 +0900
+++ /tmp/deps-arm64/lib/pkgconfig/libvncclient.pc	2022-08-11 23:41:16.000000000 +0900
@@ -8,7 +8,7 @@
 Version: 0.9.13
 Requires:
 Requires.private:
-Libs: -L\${libdir} -lvncclient
+Libs: -L\${libdir} -L${dest}/lib -lvncclient
 Libs.private:  -lsasl2.tbd -lz.tbd -llzo2.dylib -ljpeg.dylib
 Cflags: -I\${includedir}
 
EOF
