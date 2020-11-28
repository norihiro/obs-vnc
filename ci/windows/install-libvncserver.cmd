if not exist %LIBVNCPath% (
	echo obs-studio directory does not exist
	git clone https://github.com/LibVNC/libvncserver.git %LIBVNCPath%
	cd /D %LIBVNCPath%\
	git describe --tags --abbrev=0 --exclude="*-rc*" > "%LIBVNCPath%\latest-tag.txt"
    set /p OBSLatestTag=<"%LIBVNCPath%\latest-tag.txt"
)

cd /D %LIBVNCPath%\

REM below are copied from libvncserver/.appveyor.yml
if not exist deps mkdir deps
cd deps

echo Downloading nasm binary...
if %CMakeOptA% == Win32 curl -fsSL -o nasm.zip https://www.nasm.us/pub/nasm/releasebuilds/2.15.05/win32/nasm-2.15.05-win32.zip
if %CMakeOptA% == x64   curl -fsSL -o nasm.zip https://www.nasm.us/pub/nasm/releasebuilds/2.15.05/win64/nasm-2.15.05-win64.zip
7z x nasm.zip
dir nasm-2.15.05
move nasm-2.15.05 nasm
path=%Path%;%LIBVNCPath%\deps\nasm

echo Downloading lzo...
curl -fsSL -o lzo.tar.gz http://www.oberhumer.com/opensource/lzo/download/lzo-2.10.tar.gz
7z x lzo.tar.gz -so | 7z x -si -ttar > nul
move lzo-2.10 lzo
echo Building lzo...
cd lzo
mkdir build
cd build
cmake -A %CMakeOptA% ..
cmake --build . --config %build_config%
cd %LIBVNCPath%\deps

echo Downloading zlib...
curl -fsSL -o zlib.tar.gz https://github.com/madler/zlib/archive/v1.2.8.tar.gz
7z x zlib.tar.gz -so | 7z x -si -ttar > nul
move zlib-1.2.8 zlib
cd zlib
cmake -A %CMakeOptA% .
cmake --build . --config %build_config%
cd ..

echo Downloading libjpeg...
rem curl -fsSL -o libjpeg.tar.gz https://github.com/libjpeg-turbo/libjpeg-turbo/archive/2.0.4.tar.gz
curl -fsSL -o libjpeg.tar.gz https://github.com/norihiro/obs-vnc/releases/download/0.1.0/libjpeg-2.0.4-norihiro.tar.gz
7z x libjpeg.tar.gz -so | 7z x -si -ttar > nul
rem move libjpeg-turbo-2.0.4 libjpeg
echo Building libjpeg...
cd libjpeg
cmake -A %CMakeOptA% .
cmake --build . --config %build_config%
cd %LIBVNCPath%\deps

echo Downloading libpng...
curl -fsSL -o libpng.tar.gz http://prdownloads.sourceforge.net/libpng/libpng-1.6.28.tar.gz?download
7z x libpng.tar.gz -so | 7z x -si -ttar > nul
move libpng-1.6.28 libpng
echo Building libpng...
cd libpng
cmake . -A %CMakeOptA% -DZLIB_INCLUDE_DIR=%LIBVNCPath%\deps\zlib -DZLIB_LIBRARY=%LIBVNCPath%\deps\zlib\%build_config%\zlibstatic.lib
cmake --build . --config %build_config%
cd ..

:: REM Berkeley DB - required by SASL
:: curl -fsSL -o db-4.1.25.tar.gz http://download.oracle.com/berkeley-db/db-4.1.25.tar.gz
:: 7z x db-4.1.25.tar.gz -so | 7z x -si -ttar > nul
:: move db-4.1.25 db
:: cd db\build_win32
:: echo using devenv %DEVENV_EXE%
:: %DEVENV_EXE% db_dll.dsp /upgrade
:: cmake --build . --config %build_config%
:: cd ..\..

:: REM Cyrus SASL
:: curl -fsSL -o cyrus-sasl-2.1.27.tar.gz https://github.com/cyrusimap/cyrus-sasl/releases/download/cyrus-sasl-2.1.27/cyrus-sasl-2.1.27.tar.gz
:: 7z x cyrus-sasl-2.1.27.tar.gz -so | 7z x -si -ttar > nul
:: move cyrus-sasl-2.1.27 sasl
:: cd sasl
:: patch -p1 -i ..\sasl-fix-snprintf-macro.patch
:: echo using vsdevcmd %VSDEVCMD_BAT%
:: '%VSDEVCMD_BAT%'
:: nmake /f NTMakefile OPENSSL_INCLUDE=c:\OpenSSL-Win32\include OPENSSL_LIBPATH=c:\OpenSSL-Win32\lib DB_INCLUDE=c:\projects\libvncserver\deps\db\build_win32 DB_LIBPATH=c:\projects\libvncserver\deps\db\build_win32\release DB_LIB=libdb41.lib install
:: cd ..

REM go back to source root
cd ..

:: TODO: CMake cannot find 32-bit OpenSSL
set WITH_OPENSSL ON
if %CMakeOptA% == Win32 set WITH_OPENSSL OFF

echo Preparing libvncserver...
cmake --version
cmake ^
-A %CMakeOptA% ^
-DWITH_OPENSSL=%WITH_OPENSSL% ^
-DZLIB_INCLUDE_DIR=.\deps\zlib -DZLIB_LIBRARY=%LIBVNCPath%\deps\zlib\%build_config%\zlibstatic.lib ^
-DLZO_INCLUDE_DIR=.\deps\lzo\include -DLZO_LIBRARIES=%LIBVNCPath%\deps\lzo\build\%build_config%\lzo2.lib ^
-DPNG_PNG_INCLUDE_DIR=.\deps\libpng -DPNG_LIBRARY=%LIBVNCPath%\deps\libpng\%build_config%\libpng16_static.lib ^
-DJPEG_INCLUDE_DIR=%LIBVNCPath%/deps/libjpeg -DJPEG_LIBRARY=./deps/libjpeg/%build_config%/turbojpeg-static ^
-DWITH_SDL=OFF -DWITH_GTK=OFF -DWITH_SYSTEMD=OFF -DWITH_FFMPEG=OFF ^
.
cmake --build . --config %build_config%
dir /S
