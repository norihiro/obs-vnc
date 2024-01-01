#! /bin/bash

set -e

if which packagesbuild; then
	exit 0
fi

packages_url='http://s.sudre.free.fr/Software/files/Packages.dmg'
packages_hash='9d9a73a64317ea6697a380014d2e5c8c8188b59d5fb8ce8872e56cec06cd78e8'

for ((retry=5; retry>0; retry--)); do
	curl -o Packages.dmg $packages_url
	sha256sum -c <<<"$packages_hash Packages.dmg" && break
done

hdiutil attach -noverify Packages.dmg
packages_volume="$(hdiutil info -plist | grep '<string>/Volumes/Packages' | sed 's/.*<string>\(\/Volumes\/[^<]*\)<\/string>/\1/')"

sudo installer -pkg "${packages_volume}/Install Packages.pkg" -target /
hdiutil detach "${packages_volume}"
