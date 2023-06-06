#! /bin/bash

set -e

obs=28

function test_dylib
{
	echo "$1" >> $2
	if ! test -e "$1"; then
		echo "Error: File $1 not found." >&2
		return 1
	fi
	loader_path="$(dirname "$1")"
	otool -L "$1" |
	awk -v loader_path="$loader_path" '
	NR>=2 {
		dylib = $1
		sub("@loader_path", loader_path, dylib)
		sub("/lib/../lib/", "/lib/", dylib)
		sub("//", "/", dylib)
		print dylib
	}' | {
		ret=0
		while read dylib; do
			case "$obs-$dylib" in
				2?-/System/*) continue;;
				2?-/usr/lib/*) continue;; # Some libraries are not found on the host for CI.
				27-@rpath/libobs.0.dylib) continue;;
				27-@rpath/libobs-frontend-api.dylib) continue;;
				27-@executable_path/../Frameworks/Qt*.framework/Versions/5/Qt*) continue;;
				28-@rpath/libobs.framework/Versions/A/libobs) continue;;
				28-@rpath/libobs-frontend-api.1.dylib) continue;;
				28-@rpath/Qt*.framework/Versions/A/Qt*) continue;;
			esac
			if ! grep -qF "$dylib" "$2"; then
				if ! test_dylib "$dylib" "$2"; then
					echo "Error: File $1 has a dependency error." >&2
					ret=1
				fi
			fi
		done
		return $ret
	}
}

for i in "$@"; do
	case "$i" in
		-27) obs=27;;
		-28) obs=28;;
		*) test_dylib "$i" $(mktemp);;
	esac
done
