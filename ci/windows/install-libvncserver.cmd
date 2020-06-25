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

REM zlib
curl -fsSL -o zlib.tar.gz https://github.com/madler/zlib/archive/v1.2.8.tar.gz
7z x zlib.tar.gz -so | 7z x -si -ttar > nul
move zlib-1.2.8 zlib
cd zlib
cmake .
cmake --build . --config %build_config%
cd ..

REM libPNG
curl -fsSL -o libpng.tar.gz http://prdownloads.sourceforge.net/libpng/libpng-1.6.28.tar.gz?download
7z x libpng.tar.gz -so | 7z x -si -ttar > nul
move libpng-1.6.28 libpng
cd libpng
cmake . -DZLIB_INCLUDE_DIR=%LIBVNCPath%\deps\zlib -DZLIB_LIBRARY=%LIBVNCPath%\deps\zlib\%build_config%\zlibstatic.lib
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

REM build_script:
cmake --version
cmake ^
-DZLIB_INCLUDE_DIR=.\deps\zlib -DZLIB_LIBRARY=%LIBVNCPath%\deps\zlib\%build_config%\zlibstatic.lib ^
-DPNG_PNG_INCLUDE_DIR=.\deps\libpng -DPNG_LIBRARY=%LIBVNCPath%\deps\libpng\%build_config%\libpng16_static.lib ^
.
rem TODO: build in another task so that I dont run this command: cmake --build .
dir
dir deps
dir deps\zlib
dir deps\zlib\%build_config%

REM TODO: add SASL
