if "%buildWin32%" == "false" then goto skippedWin32
mkdir build32
cd build32
cmake -G "Visual Studio 16 2019" ^
-A Win32 -DCMAKE_SYSTEM_VERSION=10.0 -DQTDIR="%QTDIR32%" ^
-DLibObs_DIR="%OBSPath%\build32\libobs" ^
-DLIBOBS_INCLUDE_DIR="%OBSPath%\libobs" ^
-DCMAKE_C_FLAGS="/I%OBSPath%\deps/w32-pthreads /I%LIBVNCPath32% /I%LIBVNCPath32%/deps/zlib /I%LIBVNCPath32%/compat/msvc/" ^
-DLIBOBS_LIB="%OBSPath%\build32\libobs\%build_config%\obs.lib" ^
-DOBS_FRONTEND_LIB="%OBSPath%\build32\UI\obs-frontend-api\%build_config%\obs-frontend-api.lib" ^
-DLIBVNCCLIENT_INCLUDE_DIRS="%LIBVNCPath32%" ^
-DLIBVNCCLIENT_LIB_DIRS="%LIBVNCPath32%" ^
-DLIBVNCCLIENT_LIBRARIES="%LIBVNCPath32%\%build_config%\vncclient.lib;%OBSPath%\build32\deps\w32-pthreads\%build_config%\w32-pthreads.lib;%LIBVNCPath32%\deps\zlib\%build_config%\zlibstatic.lib;%LIBVNCPath32%\deps\lzo\build\%build_config%\lzo2.lib;%LIBVNCPath32%\deps\libpng\%build_config%\libpng16_static.lib;%LIBVNCPath32%\deps\libjpeg\%build_config%\turbojpeg-static.lib;wsock32.lib;ws2_32.lib" ^
..
cd ..
:skippedWin32

mkdir build64
cd build64
cmake -G "Visual Studio 16 2019" ^
-A x64 -DCMAKE_SYSTEM_VERSION=10.0 -DQTDIR="%QTDIR64%" ^
-DLibObs_DIR="%OBSPath%\build64\libobs" ^
-DLIBOBS_INCLUDE_DIR="%OBSPath%\libobs" ^
-DCMAKE_C_FLAGS="/I%OBSPath%\deps/w32-pthreads /I%LIBVNCPath64% /I%LIBVNCPath64%/deps/zlib /I%LIBVNCPath64%/compat/msvc/" ^
-DLIBOBS_LIB="%OBSPath%\build64\libobs\%build_config%\obs.lib" ^
-DOBS_FRONTEND_LIB="%OBSPath%\build64\UI\obs-frontend-api\%build_config%\obs-frontend-api.lib" ^
-DLIBVNCCLIENT_INCLUDE_DIRS="%LIBVNCPath64%" ^
-DLIBVNCCLIENT_LIB_DIRS="%LIBVNCPath64%" ^
-DLIBVNCCLIENT_LIBRARIES="%LIBVNCPath64%\%build_config%\vncclient.lib;%OBSPath%\build64\deps\w32-pthreads\%build_config%\w32-pthreads.lib;%LIBVNCPath64%\deps\zlib\%build_config%\zlibstatic.lib;%LIBVNCPath64%\deps\lzo\build\%build_config%\lzo2.lib;%LIBVNCPath64%\deps\libpng\%build_config%\libpng16_static.lib;%LIBVNCPath64%\deps\libjpeg\%build_config%\turbojpeg-static.lib;wsock32.lib;ws2_32.lib" ^
..

REM Import the generated includes to get the plugin's name
call "%~dp0..\ci_includes.generated.cmd"

REM Rename the solution files to something CI can pick up 
cd ..
ren "build32\%PluginName%.sln" "main.sln"
ren "build64\%PluginName%.sln" "main.sln"
