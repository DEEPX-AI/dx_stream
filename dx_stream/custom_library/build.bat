@echo off
REM ============================================================
REM  DX-Stream Custom Library Build Script (Windows)
REM  Builds postprocess and message_convert custom libraries.
REM  Usage:
REM    build.bat              -> build all custom libraries
REM    build.bat --clean      -> clean rebuild
REM ============================================================

setlocal EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
set "PROJECT_ROOT=%SCRIPT_DIR%\..\.."
pushd "%PROJECT_ROOT%" >nul
set "PROJECT_ROOT=%CD%"
popd >nul

set "PLUGIN_DIR=%PROJECT_ROOT%\gst-dxstream-plugin"
set "PLUGIN_BUILD=%PLUGIN_DIR%\builddir"
set "VCPKG_INSTALLED=%PROJECT_ROOT%\vcpkg_installed\x64-windows"

set "CLEAN_MODE="
set "BUILD_TYPE=release"
:parse_args
if "%~1"=="" goto args_done
if /I "%~1"=="--clean" (set "CLEAN_MODE=1" & shift & goto parse_args)
if /I "%~1"=="--type=debug" (set "BUILD_TYPE=debug" & shift & goto parse_args)
if /I "%~1"=="--type=release" (set "BUILD_TYPE=release" & shift & goto parse_args)
echo [ERROR] Unknown option: %~1
exit /b 1
:args_done

REM ---- Verify main plugin is built ----
if not exist "%PLUGIN_BUILD%\src\gstdxstream.dll" (
    echo [ERROR] Main plugin not built. Run build.bat in project root first.
    exit /b 1
)

REM ---- Locate Visual Studio ----
set "VCVARS="
for %%E in (Community Professional Enterprise BuildTools) do (
    if exist "%ProgramFiles%\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvarsall.bat" (
        set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvarsall.bat"
    )
)
if not defined VCVARS (
    echo [ERROR] Visual Studio 2022 not found.
    exit /b 1
)
call "%VCVARS%" x64 >nul

REM ---- Set up environment ----
if exist "!VCPKG_INSTALLED!\lib" (
    set "PKG_CONFIG_PATH=!VCPKG_INSTALLED!\lib\pkgconfig"
)
if defined GSTREAMER_1_0_ROOT_MSVC_X86_64 (
    set "PKG_CONFIG_PATH=%GSTREAMER_1_0_ROOT_MSVC_X86_64%\lib\pkgconfig;!PKG_CONFIG_PATH!"
    set "PATH=%GSTREAMER_1_0_ROOT_MSVC_X86_64%\bin;%PATH%"
) else if exist "C:\Program Files\gstreamer\1.0\msvc_x86_64" (
    set "PKG_CONFIG_PATH=C:\Program Files\gstreamer\1.0\msvc_x86_64\lib\pkgconfig;!PKG_CONFIG_PATH!"
    set "PATH=C:\Program Files\gstreamer\1.0\msvc_x86_64\bin;%PATH%"
)

REM ---- Add gstdxstream uninstalled pkg-config ----
set "PKG_CONFIG_PATH=%PLUGIN_BUILD%\meson-uninstalled;!PKG_CONFIG_PATH!"

REM ---- Create include shim for gstdxstream/ prefix ----
set "INCLUDE_SHIM=%PLUGIN_BUILD%\include-shim\gstdxstream"
if not exist "%INCLUDE_SHIM%" mkdir "%INCLUDE_SHIM%"
copy /Y "%PLUGIN_DIR%\general\dxcommon.hpp" "%INCLUDE_SHIM%\" >nul 2>&1
copy /Y "%PLUGIN_DIR%\metadata\*.hpp" "%INCLUDE_SHIM%\" >nul 2>&1
set "CXXFLAGS=/I%PLUGIN_BUILD%\include-shim"
set "CFLAGS=/I%PLUGIN_BUILD%\include-shim"

REM ---- DEEPX_SDK_DIR ----
if not defined DEEPX_SDK_DIR (
    echo [ERROR] DEEPX_SDK_DIR environment variable is not set.
    exit /b 1
)
if exist "!DEEPX_SDK_DIR!\lib\x64" (
    set "LIB=!DEEPX_SDK_DIR!\lib\x64;%LIB%"
    set "INCLUDE=!DEEPX_SDK_DIR!\include;%INCLUDE%"
)

REM ---- vcpkg paths ----
if exist "!VCPKG_INSTALLED!\lib" (
    set "PATH=!VCPKG_INSTALLED!\bin;%PATH%"
    set "INCLUDE=!VCPKG_INSTALLED!\include;%INCLUDE%"
    set "LIB=!VCPKG_INSTALLED!\lib;%LIB%"
)

echo ============================================================
echo  DX-Stream Custom Library Build
echo ============================================================
echo.

set "FAIL_COUNT=0"
set "OK_COUNT=0"

call :build_all "%SCRIPT_DIR%\postprocess_library"
call :build_all "%SCRIPT_DIR%\message_convert_library"

echo.
echo ============================================================
echo  Done: !OK_COUNT! succeeded, !FAIL_COUNT! failed.
echo ============================================================
endlocal & exit /b %FAIL_COUNT%

REM ============================================================
REM  Subroutine: iterate directories and build each
REM ============================================================
:build_all
set "TARGET_DIR=%~1"
if not exist "%TARGET_DIR%" goto :eof
for /d %%D in ("%TARGET_DIR%\*") do (
    if exist "%%D\meson.build" (
        call :build_one "%%D"
    )
)
goto :eof

REM ============================================================
REM  Subroutine: build a single custom library
REM ============================================================
:build_one
set "LIB_DIR=%~1"
set "LIB_NAME=%~nx1"
set "LIB_BUILD=%LIB_DIR%\builddir"

echo [BUILD] %LIB_NAME%

if defined CLEAN_MODE (
    if exist "%LIB_BUILD%" rmdir /S /Q "%LIB_BUILD%"
)

pushd "%LIB_DIR%" || (echo [FAIL] %LIB_NAME%: cannot enter directory & set /a FAIL_COUNT+=1 & goto :eof)

if not exist "builddir\build.ninja" (
    meson setup builddir --buildtype=%BUILD_TYPE% >nul 2>&1
    if errorlevel 1 (
        echo   [SKIP] %LIB_NAME%: meson setup failed
        popd
        set /a FAIL_COUNT+=1
        goto :eof
    )
)

meson compile -C builddir >nul 2>&1
if errorlevel 1 (
    echo   [FAIL] %LIB_NAME%: compile failed
    popd
    set /a FAIL_COUNT+=1
    goto :eof
)

echo   [OK] %LIB_NAME%
set /a OK_COUNT+=1
popd
goto :eof
