#! /bin/bash

set -ex
. build/ci/ci_includes.generated.sh

PackageVersion="$(git describe --tags --always)"

mkdir package
cd package

cp ../LICENSE ../release/data/obs-plugins/${PLUGIN_NAME}/LICENCE-${PLUGIN_NAME}.txt
cp "${LIBVNCPath}/deps/libjpeg/LICENSE.md" ../release/data/obs-plugins/${PLUGIN_NAME}/LICENSE-libjpeg.md
cp "${LIBVNCPath}/deps/libpng/LICENSE" ../release/data/obs-plugins/${PLUGIN_NAME}/LICENSE-libpng.txt
cp "${LIBVNCPath}/deps/lzo/COPYING" ../release/data/obs-plugins/${PLUGIN_NAME}/COPYING-lzo.txt
cp '/c/Program Files/OpenSSL/license.txt' ../release/data/obs-plugins/${PLUGIN_NAME}/license-OpenSSL.txt

7z a "${PLUGIN_NAME}-${PackageVersion}-obs$1-Windows.zip" ../release/*
cmd.exe <<EOF
iscc ..\\build\\installer-Windows.generated.iss /O. /F"${PLUGIN_NAME}-${PackageVersion}-obs$1-Windows-Installer"
EOF
sleep 2 && echo

sha1sum \
	"${PLUGIN_NAME}-${PackageVersion}-obs$1-Windows.zip" \
	"${PLUGIN_NAME}-${PackageVersion}-obs$1-Windows-Installer.exe"
