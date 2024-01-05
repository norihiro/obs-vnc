call "%~dp0..\ci_includes.generated.cmd"

mkdir package
cd package

git describe --tags --always > package-version.txt
set /p PackageVersion=<package-version.txt
del package-version.txt

copy ..\LICENSE          ..\release\data\obs-plugins\%PluginName%\LICENCE-%PluginName%.txt
copy %LIBVNCPath%\deps\libjpeg\LICENSE.md ..\release\data\obs-plugins\%PluginName%\LICENSE-libjpeg.md
copy %LIBVNCPath%\deps\libpng\LICENSE ..\release\data\obs-plugins\%PluginName%\LICENSE-libpng.txt
copy %LIBVNCPath%\deps\lzo\COPYING ..\release\data\obs-plugins\%PluginName%\COPYING-lzo.txt
copy "c:\Program Files\OpenSSL\license.txt" ..\release\data\obs-plugins\%PluginName%\license-OpenSSL.txt

REM Package ZIP archive
7z a "%PluginName%-%PackageVersion%-obs%1-Windows.zip" "..\release\*"

REM Build installer
iscc ..\installer\installer-Windows.generated.iss /O. /F"%PluginName%-%PackageVersion%-obs%1-Windows-Installer"

certutil.exe -hashfile "%PluginName%-%PackageVersion%-obs%1-Windows.zip" SHA1
certutil.exe -hashfile "%PluginName%-%PackageVersion%-obs%1-Windows-Installer.exe" SHA1
