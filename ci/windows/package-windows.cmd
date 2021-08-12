call "%~dp0..\ci_includes.generated.cmd"

mkdir package
cd package

git describe --tags --always > package-version.txt
set /p PackageVersion=<package-version.txt
del package-version.txt

copy ..\LICENSE          ..\release\data\obs-plugins\%PluginName%\LICENCE-%PluginName%.txt
copy %LIBVNCPath64%\deps\libjpeg\LICENSE.md ..\release\data\obs-plugins\%PluginName%\LICENSE-libjpeg.md
copy %LIBVNCPath64%\deps\libpng\LICENSE ..\release\data\obs-plugins\%PluginName%\LICENSE-libpng.txt
copy %LIBVNCPath64%\deps\lzo\COPYING ..\release\data\obs-plugins\%PluginName%\COPYING-lzo.txt

REM Package ZIP archive
7z a "%PluginName%-%PackageVersion%-Windows.zip" "..\release\*"

REM Build installer
iscc ..\installer\installer-Windows.generated.iss /O. /F"%PluginName%-%PackageVersion%-Windows-Installer"
