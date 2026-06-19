@echo off
setlocal

set QT_DIR=D:\Qt\6.11.1\mingw_64
set MINGW_BIN=D:\Qt\Tools\mingw1310_64\bin
set CMAKE_BIN=D:\Qt\Tools\CMake_64\bin

set PATH=%MINGW_BIN%;%QT_DIR%\bin;%CMAKE_BIN%;%PATH%

if not exist build mkdir build
cd build

cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="%QT_DIR%" -DCMAKE_BUILD_TYPE=Release ..
if errorlevel 1 exit /b 1

cmake --build . --config Release
if errorlevel 1 exit /b 1

echo.
echo Build done.
echo   Run:        dist\bin\qt_bilibili_live.exe
echo   Package:    cd .. ^&^& package.bat

endlocal
