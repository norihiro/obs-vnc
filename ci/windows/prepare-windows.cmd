if "%buildWin32%" == "false" then goto skippedWin32
mkdir build32
cd build32
cmake -G "Visual Studio 16 2019" -A Win32 -DCMAKE_SYSTEM_VERSION=10.0 -DQTDIR="%QTDIR32%" -DLibObs_DIR="%OBSPath%\build32\libobs" -DLIBOBS_INCLUDE_DIR="%OBSPath%\libobs" -DCMAKE_C_FLAGS="/I%OBSPath%\deps/w32-pthreads" -DLIBOBS_LIB="%OBSPath%\build32\libobs\%build_config%\obs.lib" -DOBS_FRONTEND_LIB="%OBSPath%\build32\UI\obs-frontend-api\%build_config%\obs-frontend-api.lib" -DLIBVNCCLIENT_INCLUDE_DIRS="%LIBVNCPath%" -DLIBVNCCLIENT_LIB_DIRS="%LIBVNCPath%" ..
cd ..
:skippedWin32

mkdir build64

cd build64
cmake -G "Visual Studio 16 2019" ^
-A x64 -DCMAKE_SYSTEM_VERSION=10.0 -DQTDIR="%QTDIR64%" ^
-DLibObs_DIR="%OBSPath%\build64\libobs" ^
-DLIBOBS_INCLUDE_DIR="%OBSPath%\libobs" ^
-DCMAKE_C_FLAGS="/I%OBSPath%\deps/w32-pthreads /I%LIBVNCPath% /I%LIBVNCPath%/deps/zlib /I%LIBVNCPath%/compat/msvc/" ^
-DLIBOBS_LIB="%OBSPath%\build64\libobs\%build_config%\obs.lib" ^
-DOBS_FRONTEND_LIB="%OBSPath%\build64\UI\obs-frontend-api\%build_config%\obs-frontend-api.lib" ^
-DLIBVNCCLIENT_INCLUDE_DIRS="%LIBVNCPath%" ^
-DLIBVNCCLIENT_LIB_DIRS="%LIBVNCPath%" ^
-DLIBVNCCLIENT_LIBRARIES="%LIBVNCPath%\%build_config%\vncclient.lib" ^
-DPTHREAD_LIBRARIES="%OBSPath%\build64\deps\w32-pthreads\%build_config%\w32-pthreads.lib" ^
-DZLIB_LIB="%LIBVNCPath%\deps\zlib\%build_config%\zlibstatic.lib" ^
-DLIBPNG_LIB="%LIBVNCPath%\deps\libpng\%build_config%\libpng16_static.lib" ^
..

REM Import the generated includes to get the plugin's name
call "%~dp0..\ci_includes.generated.cmd"

REM Rename the solution files to something CI can pick up 
cd ..
ren "build32\%PluginName%.sln" "main.sln"
ren "build64\%PluginName%.sln" "main.sln"
