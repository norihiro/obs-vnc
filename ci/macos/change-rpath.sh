#! /bin/bash

libdir=''
obsver=''
licdir=''

while (($# > 0)); do
	case "$1" in
		-lib)
			libdir="$2"
			shift 2;;
		-obs)
			obsver="$2"
			shift 2;;
		-license)
			licdir="$2"
			shift 2;;
		*)
			break ;;
	esac
done

set -e

function copy_local_dylib
{
	local dylib
	t=$(mktemp)
	otool -L $1 > $t
	awk '/^	\/(usr\/local\/(opt|Cellar)|opt\/homebrew)\/.*\.dylib/{print $1}' $t |
	while read -r dylib; do
		echo "Changing dependency $1 -> $dylib"
		local b=$(basename $dylib)
		if test ! -e $libdir/$b; then
			mkdir -p $libdir
			cp $dylib $libdir
			chmod +rwx $libdir/$b
			install_name_tool -id "@loader_path/$b" $libdir/$b
			copy_local_dylib $libdir/$b
		fi
		install_name_tool -change "$dylib" "@loader_path/../$libdir/$b" $1

		if test -n "$licdir"; then
			find "$(cd "$(dirname "$dylib")/.." && pwd -P)" |
			awk -v licdir="$licdir" -v FS='/' '
			/(COPYING|LICENSE)(\.md|\.txt)?$/ {
				for (i=1; i<NF; i++) {
					if ($i ~ /^[a-zA-Z]/)
						name = $i
				}
				system("cp "$0" "licdir"/"name"-"$NF)
			}
			'
		fi
	done
	rm -f "$t"
}

function change_obs27_libs
{
	# obs-frontend-api:
	# OBS 27.2 provides only `libobs-frontend-api.dylib`.
	# OBS 28.0 will provide `libobs-frontend-api.1.dylib` and `libobs-frontend-api.dylib`.
	# libobs:
	# Both OBS 27.2 and 28.0 provides `libobs.dylib`, `libobs.0.dylib`, `libobs.framework/Versions/A/libobs`.

	install_name_tool \
		-change @rpath/QtWidgets.framework/Versions/5/QtWidgets \
			@executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets \
		-change @rpath/QtGui.framework/Versions/5/QtGui \
			@executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui \
		-change @rpath/QtCore.framework/Versions/5/QtCore \
			@executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore \
		-change @rpath/libobs.framework/Versions/A/libobs \
			@rpath/libobs.0.dylib \
		-change @rpath/libobs-frontend-api.0.dylib \
			@rpath/libobs-frontend-api.dylib \
		"$1"
}

for i in "$@"; do
	case "$obsver" in
		27 | 27.*)
			change_obs27_libs "$i"
			;;
		28 | 28.*)
			: # Not necessary to change dylib paths for OBS 28
			;;
	esac
	copy_local_dylib "$i"
done
