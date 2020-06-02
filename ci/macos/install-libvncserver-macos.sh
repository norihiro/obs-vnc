#! /bin/sh

OSTYPE=$(uname)

if [ "${OSTYPE}" != "Darwin" ]; then
    echo "[Error] macOS libvncserver build script can be run on Darwin-type OS only."
    exit 1
fi

HAS_CMAKE=$(type cmake 2>/dev/null)
HAS_GIT=$(type git 2>/dev/null)

if [ "${HAS_CMAKE}" = "" ]; then
    echo "[Error] CMake not installed - please run 'install-dependencies-macos.sh' first."
    exit 1
fi

if [ "${HAS_GIT}" = "" ]; then
    echo "[Error] Git not installed - please install Xcode developer tools or via Homebrew."
    exit 1
fi

cd ..
echo "=> Cloning libvncserver from GitHub.."
git clone https://github.com/LibVNC/libvncserver.git
cd libvncserver
OBSLatestTag=$(git describe --tags --abbrev=0)
git checkout $OBSLatestTag
mkdir build && cd build
echo "=> Building libvncserver.."
cmake .. \
	-DCMAKE_PREFIX_PATH=/usr/local/opt/qt/lib/cmake \
	-DOPENSSL_ROOT_DIR=/usr/local/opt/openssl \
	-DWITH_OPENSSL=OFF -DWITH_GNUTLS=OFF -DWITH_GCRYPT=OFF \
&& make -j4

# for debugging
set -x
pwd
find .
cat libvncserver.pc
