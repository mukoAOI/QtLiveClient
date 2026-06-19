@echo off
setlocal

set QT_DIR=D:\Qt\6.11.1\mingw_64
set MINGW_BIN=D:\Qt\Tools\mingw1310_64\bin
set CMAKE_BIN=D:\Qt\Tools\CMake_64\bin
set NSIS_DIR=C:\Program Files (x86)\NSIS

set PATH=%MINGW_BIN%;%QT_DIR%\bin;%CMAKE_BIN%;%NSIS_DIR%;%PATH%

set "INSTALLER=qt_bilibili_live-1.0.0-win64.exe"

if not exist build mkdir build
pushd build

cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="%QT_DIR%" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%CD%\dist" ..
if errorlevel 1 exit /b 1

cmake --build . --config Release
if errorlevel 1 exit /b 1

cmake --install .
if errorlevel 1 exit /b 1

if exist _CPack_Packages rmdir /s /q _CPack_Packages

cpack -G NSIS
if errorlevel 1 (
    set "NSIS_STAGE=%CD%\_CPack_Packages\win64\NSIS"
    if not exist "%NSIS_STAGE%\project.nsi" (
        echo NSIS package failed and staging directory was not created.
        echo Ensure NSIS is installed: https://nsis.sourceforge.io/Download
        exit /b 1
    )
    echo Retrying NSIS build with InstallOptions.ini workaround...
    copy /Y "%CD%\NSIS.InstallOptions.ini" "%NSIS_STAGE%\" >nul
    pushd "%NSIS_STAGE%"
    makensis project.nsi
    set "NSIS_RC=%ERRORLEVEL%"
    popd
    if not "%NSIS_RC%"=="0" exit /b 1
    copy /Y "%NSIS_STAGE%\%INSTALLER%" "%CD%\"
)

if not exist "%CD%\%INSTALLER%" (
    echo Installer exe was not created.
    exit /b 1
)

echo.
echo Installer created:
echo   %CD%\%INSTALLER%
echo.
echo Portable dir:
echo   %CD%\dist\bin\

popd
endlocal
