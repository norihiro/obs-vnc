#!/bin/bash

set -e

script_dir=$(dirname "$0")
source "$script_dir/../ci_includes.generated.sh"

OSTYPE=$(uname)

if [ "${OSTYPE}" != "Darwin" ]; then
    echo "[Error] macOS package script can be run on Darwin-type OS only."
    exit 1
fi

echo "=> Preparing package build"
GIT_TAG=$(git describe --tags --long --always)
GIT_TAG_ONLY=$(git describe --tags --always)

FILENAME_UNSIGNED="$PLUGIN_NAME-${GIT_TAG}-Unsigned.pkg"
FILENAME="$PLUGIN_NAME-${GIT_TAG}.pkg"

echo "=> Modifying $PLUGIN_NAME.so"
mkdir -p lib
for dylib in \
	/usr/local/opt/lzo/lib/liblzo2.2.dylib \
	/usr/local/opt/jpeg/lib/libjpeg.9.dylib \
	/usr/local/opt/libpng/lib/libpng16.16.dylib
do
	cp $dylib lib/
	b=$(basename $dylib)
	chmod +rw lib/$b
	install_name_tool -id "@loader_path/$b" lib/$b
	install_name_tool -change "$dylib" "@loader_path/../lib/$b" ./build/$PLUGIN_NAME.so
done

install_name_tool \
	-change /tmp/obsdeps/lib/QtWidgets.framework/Versions/5/QtWidgets \
		@executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets \
	-change /tmp/obsdeps/lib/QtGui.framework/Versions/5/QtGui \
		@executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui \
	-change /tmp/obsdeps/lib/QtCore.framework/Versions/5/QtCore \
		@executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore \
	./build/$PLUGIN_NAME.so

# Check if replacement worked
for dylib in ./build/$PLUGIN_NAME.so lib/*.dylib ; do
	echo "=> Dependencies for $(basename $dylib)"
	otool -L $dylib
	echo "=> Search paths written in $(basename $dylib)"
	otool -l $dylib
done

if [[ "$RELEASE_MODE" == "True" ]]; then
	echo "=> Signing plugin binary: $PLUGIN_NAME.so"
	codesign --sign "$CODE_SIGNING_IDENTITY" ./build/$PLUGIN_NAME.so
else
	echo "=> Skipped plugin codesigning"
fi

echo "=> ZIP package build"
ziproot=package-zip/$PLUGIN_NAME
zipfile=${PLUGIN_NAME}-${GIT_TAG}-macos.zip
rm -rf $ziproot/
mkdir -p $ziproot/bin
cp ./build/$PLUGIN_NAME.so $ziproot/bin/
cp -a data $ziproot/
mkdir -p ./release
chmod +x lib/*.dylib
mv lib $ziproot/
(cd package-zip && zip -r ../release/$zipfile $PLUGIN_NAME)

echo "=> DMG package build"
if pip3 install dmgbuild || pip install dmgbuild; then
	sed \
		-e "s;%PLUGIN_NAME%;$PLUGIN_NAME;g" \
		-e "s;%VERSION%;${GIT_TAG_ONLY};g" \
		-e "s;%PLUGIN_ROOT%;$ziproot;g" \
		< ci/macos/package-dmg.json.template > package-dmg.json
	dmgbuild "$PLUGIN_NAME ${GIT_TAG}" "release/${PLUGIN_NAME}-${GIT_TAG}-macos.dmg" -s ./package-dmg.json
fi

# echo "=> Actual package build"
# packagesbuild ./installer/installer-macOS.generated.pkgproj

# echo "=> Renaming $PLUGIN_NAME.pkg to $FILENAME"
# mv ./release/$PLUGIN_NAME.pkg ./release/$FILENAME_UNSIGNED

if [[ "$RELEASE_MODE" == "True" ]]; then
	echo "=> Signing installer: $FILENAME"
	productsign \
		--sign "$INSTALLER_SIGNING_IDENTITY" \
		./release/$FILENAME_UNSIGNED \
		./release/$FILENAME
	rm ./release/$FILENAME_UNSIGNED

	echo "=> Submitting installer $FILENAME for notarization"
	zip -r ./release/$FILENAME.zip ./release/$FILENAME
	UPLOAD_RESULT=$(xcrun altool \
		--notarize-app \
		--primary-bundle-id "$MACOS_BUNDLEID" \
		--username "$AC_USERNAME" \
		--password "$AC_PASSWORD" \
		--asc-provider "$AC_PROVIDER_SHORTNAME" \
		--file "./release/$FILENAME.zip")
	rm ./release/$FILENAME.zip

	REQUEST_UUID=$(echo $UPLOAD_RESULT | awk -F ' = ' '/RequestUUID/ {print $2}')
	echo "Request UUID: $REQUEST_UUID"

	echo "=> Wait for notarization result"
	# Pieces of code borrowed from rednoah/notarized-app
	while sleep 30 && date; do
		CHECK_RESULT=$(xcrun altool \
			--notarization-info "$REQUEST_UUID" \
			--username "$AC_USERNAME" \
			--password "$AC_PASSWORD" \
			--asc-provider "$AC_PROVIDER_SHORTNAME")
		echo $CHECK_RESULT

		if ! grep -q "Status: in progress" <<< "$CHECK_RESULT"; then
			echo "=> Staple ticket to installer: $FILENAME"
			xcrun stapler staple ./release/$FILENAME
			break
		fi
	done
else
	echo "=> Skipped installer codesigning and notarization"
fi
