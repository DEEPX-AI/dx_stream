@echo off
chcp 65001 >nul
REM ============================================================
REM  DX-Stream Windows Build Script
REM  Builds the full DX-Stream project:
REM    1) gst-dxstream GStreamer plugin (build + install)
REM    2) Custom libraries (postprocess, message_convert)
REM    3) Applications (usermeta, kafka, mqtt)
REM    4) Python bindings (pydxs)
REM  Usage:
REM    build.bat                -> configure (if needed) and compile all
REM    build.bat --clean        -> remove builddirs, then configure & compile
REM    build.bat --dxvnpu       -> enable DXVNPU elements
REM    build.bat --type=debug   -> debug build
REM    build.bat --plugin-only  -> build only the GStreamer plugin
REM    build.bat --uninstall    -> remove install/ and env vars
REM ============================================================

setlocal EnableDelayedExpansion

set "PROJECT_ROOT=%~dp0"
if "%PROJECT_ROOT:~-1%"=="\" set "PROJECT_ROOT=%PROJECT_ROOT:~0,-1%"
set "PLUGIN_DIR=%PROJECT_ROOT%\gst-dxstream-plugin"
set "BUILD_DIR=%PLUGIN_DIR%\builddir"
set "PREFIX=%PROJECT_ROOT%\install"

set "CLEAN_MODE="
set "DXVNPU_MODE="
set "PLUGIN_ONLY="
set "UNINSTALL_MODE="
set "BUILD_TYPE=release"
:parse_args
if "%~1"=="" goto args_done
if /I "%~1"=="--clean" (
    set "CLEAN_MODE=1"
    shift
    goto parse_args
)
if /I "%~1"=="--dxvnpu" (
    set "DXVNPU_MODE=1"
    shift
    goto parse_args
)
if /I "%~1"=="--plugin-only" (
    set "PLUGIN_ONLY=1"
    shift
    goto parse_args
)
if /I "%~1"=="--uninstall" (
    set "UNINSTALL_MODE=1"
    shift
    goto parse_args
)
if /I "%~1"=="--type=debug" (
    set "BUILD_TYPE=debug"
    shift
    goto parse_args
)
if /I "%~1"=="--type=release" (
    set "BUILD_TYPE=release"
    shift
    goto parse_args
)
if /I "%~1"=="--help" (
    echo Usage: build.bat [--clean] [--dxvnpu] [--type=debug^|release] [--plugin-only] [--uninstall]
    echo.
    echo Options:
    echo   --clean          Remove builddirs before configure
    echo   --dxvnpu         Enable DXVNPU elements [requires dxvnpu library]
    echo   --type=TYPE      Build type: debug or release [default: release]
    echo   --plugin-only    Build only the GStreamer plugin (skip libs/apps/pydxs^)
    echo   --uninstall      Remove install directory and registered environment variables
    exit /b 0
)
echo [ERROR] Unknown option: %~1
exit /b 1
:args_done

REM ---- Handle uninstall ----
if defined UNINSTALL_MODE (
    call :do_uninstall
    endlocal
    exit /b 0
)

REM ---- Locate Visual Studio (vcvarsall.bat) ----
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VCVARS="
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        if exist "%%i\VC\Auxiliary\Build\vcvarsall.bat" set "VCVARS=%%i\VC\Auxiliary\Build\vcvarsall.bat"
    )
)
if not defined VCVARS (
    for %%E in (Community Professional Enterprise BuildTools) do (
        if exist "%ProgramFiles%\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvarsall.bat" (
            set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvarsall.bat"
        )
    )
)
if not defined VCVARS (
    echo [ERROR] Could not find Visual Studio 2022 vcvarsall.bat
    echo         Install "Desktop development with C++" workload.
    exit /b 1
)
echo [INFO] Using vcvarsall: %VCVARS%
call "%VCVARS%" x64 >nul
if errorlevel 1 (
    echo [ERROR] Failed to initialize MSVC environment
    exit /b 1
)

REM ---- DEEPX SDK (inference backend) ----
if not defined DEEPX_SDK_DIR (
    echo [ERROR] DEEPX_SDK_DIR environment variable is not set.
    echo         Set DEEPX_SDK_DIR to the DEEPX SDK install directory.
    echo         Expected: %%DEEPX_SDK_DIR%%\include\ and %%DEEPX_SDK_DIR%%\lib\dxrt.lib
    exit /b 1
)
set "DXRT_SDK_DIR=%DEEPX_SDK_DIR%"
if not exist "%DXRT_SDK_DIR%\lib\x64\dxrt.lib" (
    echo [ERROR] dxrt.lib not found at %DXRT_SDK_DIR%\lib\x64
    echo         Check that DEEPX_SDK_DIR points to the correct SDK directory.
    exit /b 1
)
set "DXRT_RUNTIME_DIR="
if exist "%DEEPX_SDK_DIR%\bin" set "DXRT_RUNTIME_DIR=%DEEPX_SDK_DIR%\bin"
set "INCLUDE=%DXRT_SDK_DIR%\include;%INCLUDE%"
set "LIB=%DXRT_SDK_DIR%\lib\x64;%LIB%"
echo [INFO] dxrt sdk: %DXRT_SDK_DIR%
if defined DXRT_RUNTIME_DIR echo [INFO] dxrt runtime: %DXRT_RUNTIME_DIR%

REM ---- DEEPX dxvnpu (optional, required with --dxvnpu) ----
if defined DXVNPU_MODE (
    if not defined DXVNPU_DIR (
        echo [ERROR] DXVNPU_DIR environment variable is not set.
        echo         Set DXVNPU_DIR to the dxvnpu install directory.
        echo         Expected: %%DXVNPU_DIR%%\include\ and %%DXVNPU_DIR%%\lib\dxvnpu.lib
        exit /b 1
    )
    if not exist "!DXVNPU_DIR!\lib\dxvnpu.lib" (
        echo [ERROR] dxvnpu.lib not found at !DXVNPU_DIR!\lib
        exit /b 1
    )
    set "INCLUDE=!DXVNPU_DIR!\include;%INCLUDE%"
    set "LIB=!DXVNPU_DIR!\lib;%LIB%"
    echo [INFO] dxvnpu: !DXVNPU_DIR!
)

REM ---- vcpkg dependencies ----
set "VCPKG_INSTALLED=%PROJECT_ROOT%\vcpkg_installed\x64-windows"
if exist "!VCPKG_INSTALLED!\lib" (
    set "PKG_CONFIG_PATH=!VCPKG_INSTALLED!\lib\pkgconfig;%PKG_CONFIG_PATH%"
    set "CMAKE_PREFIX_PATH=!VCPKG_INSTALLED!;%CMAKE_PREFIX_PATH%"
    set "PATH=!VCPKG_INSTALLED!\bin;%PATH%"
    set "INCLUDE=!VCPKG_INSTALLED!\include;%INCLUDE%"
    set "LIB=!VCPKG_INSTALLED!\lib;%LIB%"
    echo [INFO] vcpkg deps: !VCPKG_INSTALLED!
) else (
    echo [ERROR] vcpkg dependencies not found. Run install.bat first.
    exit /b 1
)

REM ---- GStreamer dev environment ----
if not defined GSTREAMER_1_0_ROOT_MSVC_X86_64 (
    if exist "C:\Program Files\gstreamer\1.0\msvc_x86_64" (
        set "GSTREAMER_1_0_ROOT_MSVC_X86_64=C:\Program Files\gstreamer\1.0\msvc_x86_64"
    )
)
if defined GSTREAMER_1_0_ROOT_MSVC_X86_64 (
    set "PATH=%GSTREAMER_1_0_ROOT_MSVC_X86_64%\bin;%PATH%"
    set "PKG_CONFIG_PATH=%GSTREAMER_1_0_ROOT_MSVC_X86_64%\lib\pkgconfig;%PKG_CONFIG_PATH%"
    echo [INFO] GStreamer: %GSTREAMER_1_0_ROOT_MSVC_X86_64%
) else (
    echo [WARN] GStreamer MSVC dev not found; meson setup will fail.
)

REM ---- Clean ----
if defined CLEAN_MODE (
    if exist "%BUILD_DIR%" (
        echo [INFO] Removing %BUILD_DIR%
        rmdir /S /Q "%BUILD_DIR%"
    )
    for /D %%D in ("%PROJECT_ROOT%\dx_stream\custom_library\postprocess_library\*") do (
        if exist "%%D\builddir" rmdir /S /Q "%%D\builddir"
    )
    if exist "%PROJECT_ROOT%\dx_stream\custom_library\message_convert_library\dx_msgconvl\builddir" (
        rmdir /S /Q "%PROJECT_ROOT%\dx_stream\custom_library\message_convert_library\dx_msgconvl\builddir"
    )
    for /D %%D in ("%PROJECT_ROOT%\dx_stream\apps\*") do (
        if exist "%%D\builddir" rmdir /S /Q "%%D\builddir"
    )
    REM Clean pydxs build cache
    if exist "%PROJECT_ROOT%\bindings\python\pydxs\build" rmdir /S /Q "%PROJECT_ROOT%\bindings\python\pydxs\build"
    if exist "%PROJECT_ROOT%\bindings\python\pydxs\dist" rmdir /S /Q "%PROJECT_ROOT%\bindings\python\pydxs\dist"
    if exist "%PROJECT_ROOT%\bindings\python\pydxs\src\pydxs.egg-info" rmdir /S /Q "%PROJECT_ROOT%\bindings\python\pydxs\src\pydxs.egg-info"
)

REM ---- Tool checks ----
where meson >nul 2>nul || (echo [ERROR] meson not in PATH & exit /b 1)
where ninja >nul 2>nul || (echo [ERROR] ninja not in PATH & exit /b 1)
where pkg-config >nul 2>nul || (echo [WARN] pkg-config not in PATH; relying on GStreamer's bundled one)

REM ---- Meson options ----
set "MESON_OPTS=--prefix="%PREFIX%" --buildtype=%BUILD_TYPE%"
if defined DXVNPU_MODE (
    set "MESON_OPTS=%MESON_OPTS% -Ddxvnpu_flag=true"
)

REM ---- Configure ----
pushd "%PLUGIN_DIR%" || exit /b 1
if exist "%BUILD_DIR%\build.ninja" (
    echo [INFO] meson setup --reconfigure [buildtype=%BUILD_TYPE%]
    meson setup "%BUILD_DIR%" --reconfigure %MESON_OPTS%
) else (
    echo [INFO] meson setup [buildtype=%BUILD_TYPE%]
    meson setup "%BUILD_DIR%" %MESON_OPTS%
)
if errorlevel 1 (
    popd
    echo [ERROR] meson setup failed
    exit /b 1
)

REM ---- Compile plugin ----
echo [INFO] meson compile (gst-dxstream-plugin)
meson compile -C "%BUILD_DIR%"
set "RC=%ERRORLEVEL%"
popd

if not "%RC%"=="0" (
    echo [ERROR] meson compile failed [exit %RC%]
    exit /b %RC%
)

REM ---- Install plugin (makes gstdxstream.pc available) ----
echo [INFO] meson install (gst-dxstream-plugin)
meson install -C "%BUILD_DIR%" --no-rebuild
if errorlevel 1 (
    echo [ERROR] meson install failed
    exit /b 1
)

REM ---- Invalidate cached GStreamer plugin registry ----
REM gstdxstream.dll was just rebuilt in-place; GStreamer's registry cache keys
REM off directory mtime and can keep serving a stale/blacklisted entry for the
REM same filename even after the file content changed. Force a rescan on the
REM next pipeline run by deleting this install's own scoped registry cache.
if exist "%PREFIX%\gst-registry.bin" (
    echo [INFO] Invalidating stale GStreamer plugin registry cache
    del /f /q "%PREFIX%\gst-registry.bin" >nul 2>&1
)

REM ---- Collect runtime DLLs into install\bin\ ----
echo [INFO] Collecting runtime DLLs into install\bin\...
if not exist "%PREFIX%\bin" mkdir "%PREFIX%\bin"

REM Copy plugin DLL to bin/ as well (for DLL co-location)
copy /Y "%PREFIX%\lib\gstreamer-1.0\gstdxstream.dll" "%PREFIX%\bin\" >nul 2>&1

REM vcpkg runtime DLLs
set "VCPKG_BIN=%PROJECT_ROOT%\vcpkg_installed\x64-windows\bin"
if exist "!VCPKG_BIN!" (
    for %%F in ("!VCPKG_BIN!\*.dll") do (
        set "DLL_NAME=%%~nxF"
        if /I not "!DLL_NAME!"=="legacy.dll" (
        if /I not "!DLL_NAME!"=="libssl-3-x64.dll" (
        if /I not "!DLL_NAME!"=="libcrypto-3-x64.dll" (
        if /I not "!DLL_NAME!"=="turbojpeg.dll" (
            copy /Y "%%F" "%PREFIX%\bin\" >nul 2>&1
        ))))
    )
)

REM dxrt runtime DLLs are not copied; DXRT is provided by external SDK/runtime.

REM ---- Update PKG_CONFIG_PATH for downstream projects ----
set "PKG_CONFIG_PATH=%PREFIX%\lib\pkgconfig;%PKG_CONFIG_PATH%"
set "INCLUDE=%PREFIX%\include;%INCLUDE%"
set "LIB=%PREFIX%\lib;%LIB%"

if defined PLUGIN_ONLY (
    echo.
    echo [OK] Plugin build finished. Artifacts in: %BUILD_DIR%
    echo      Installed to: %PREFIX%
    endlocal
    exit /b 0
)

REM ============================================================
REM  Phase 2: Custom Libraries
REM ============================================================
set "DXSTREAM_DIR=%PROJECT_ROOT%\dx_stream"
set "SUB_MESON_OPTS=--prefix="%PREFIX%" --buildtype=%BUILD_TYPE%"
set "BUILD_FAILED=0"

echo.
echo [INFO] === Building custom libraries ===

REM ---- Postprocess libraries ----
pushd "%DXSTREAM_DIR%\custom_library\postprocess_library" 2>nul && (
    for /D %%D in (*) do (
        if exist "%%D\meson.build" (
            call :build_subproject "!CD!\%%D" "%%D"
            if !errorlevel! neq 0 set "BUILD_FAILED=1"
        )
    )
    popd
)

REM ---- Message convert library ----
set "MSGCONV_DIR=%DXSTREAM_DIR%\custom_library\message_convert_library\dx_msgconvl"
if exist "%MSGCONV_DIR%\meson.build" (
    call :build_subproject "%MSGCONV_DIR%" "dx_msgconvl"
    if !errorlevel! neq 0 set "BUILD_FAILED=1"
)

REM ============================================================
REM  Phase 3: Applications
REM ============================================================
echo.
echo [INFO] === Building applications ===

pushd "%DXSTREAM_DIR%\apps" 2>nul && (
    for /D %%D in (*) do (
        if exist "%%D\meson.build" (
            call :build_subproject "!CD!\%%D" "%%D"
            if !errorlevel! neq 0 set "BUILD_FAILED=1"
        )
    )
    popd
)

REM ============================================================
REM  Phase 4: Python Bindings (pydxs)
REM ============================================================
echo.
echo [INFO] === Building Python bindings (pydxs) ===
echo [INFO]   NOTE: GStreamer MSI bundles PyGObject compiled for a specific Python version.
echo [INFO]   If Python version mismatches, some pydxs tests will be skipped. See run_pydxs.bat.

set "PYDXS_DIR=%PROJECT_ROOT%\bindings\python\pydxs"
if not exist "!PYDXS_DIR!\setup.py" (
    echo [SKIP] pydxs source not found.
    goto :pydxs_done
)

where python >nul 2>nul
if !errorlevel! neq 0 (
    echo [SKIP] Python not found in PATH. Skipping pydxs.
    goto :pydxs_done
)

set "VENV_PATH=%PROJECT_ROOT%\venv-dx_stream"

for /f "usebackq tokens=*" %%V in (`python -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')"`) do set "PYTHON_VERSION=%%V"

set "VENV_PYTHON_VERSION="
if exist "!VENV_PATH!\Scripts\python.exe" for /f "usebackq tokens=*" %%V in (`"!VENV_PATH!\Scripts\python.exe" -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')" 2^>nul`) do set "VENV_PYTHON_VERSION=%%V"

if exist "!VENV_PATH!\Scripts\python.exe" if not "!VENV_PYTHON_VERSION!"=="!PYTHON_VERSION!" (
    echo [WARN] Existing venv Python !VENV_PYTHON_VERSION! differs from system Python !PYTHON_VERSION!. Recreating venv...
    rmdir /S /Q "!VENV_PATH!"
)

if not exist "!VENV_PATH!\Scripts\activate.bat" (
    echo [INFO] Creating virtual environment: !VENV_PATH!
    python -m venv --system-site-packages "!VENV_PATH!"
    if !errorlevel! neq 0 (
        echo [WARN] Failed to create venv. Skipping pydxs.
        goto :pydxs_done
    )
)

echo [INFO] Activating virtual environment
call "!VENV_PATH!\Scripts\activate.bat"

echo [INFO] Installing pydxs build dependencies...
python -m pip install --quiet --upgrade pip setuptools wheel pybind11
if !errorlevel! neq 0 (
    echo [WARN] Failed to install build dependencies. Skipping pydxs.
    call deactivate 2>nul
    goto :pydxs_done
)

REM Ensure gstdxstream.dll is findable during import verification
set "PATH=%PREFIX%\bin;%PREFIX%\lib\gstreamer-1.0;!PATH!"
if defined DXRT_RUNTIME_DIR if exist "!DXRT_RUNTIME_DIR!" set "PATH=!DXRT_RUNTIME_DIR!;!PATH!"

pushd "!PYDXS_DIR!"
echo [INFO] Installing pydxs...
python -m pip install --quiet --no-build-isolation .
if !errorlevel! equ 0 (
    echo [ OK ] pydxs
    python -c "import os, sys; dirs = [r'%PREFIX%\bin', r'%PREFIX%\lib\gstreamer-1.0', r'%GSTREAMER_1_0_ROOT_MSVC_X86_64%\bin', r'%DXRT_RUNTIME_DIR%']; [os.add_dll_directory(d) for d in dirs if d and os.path.isdir(d)]; import pydxs; print('[INFO] Import verification: OK')" 2>&1
    if !errorlevel! neq 0 (
        echo [FAIL] pydxs import verification failed. Check DLL dependencies.
        set "BUILD_FAILED=1"
    )
) else (
    echo [FAIL] pydxs
    echo [INFO] Retry with verbose output: pip install -v .
    set "BUILD_FAILED=1"
)
popd

call deactivate 2>nul

:pydxs_done

REM ---- Summary ----
echo.
if "!BUILD_FAILED!"=="1" (
    echo [WARN] Build completed with errors. Check output above.
    endlocal
    exit /b 1
) else (
    echo [OK] Full build finished successfully.
    echo      Plugin installed to: %PREFIX%
)

REM ---- Register environment variables ----
call :setup_env

echo.
echo ==========================================================
echo  Next steps:
echo    - Run demo now:       run_demo.bat
echo    - Manual testing:     Open a NEW terminal, then:
echo                          gst-inspect-1.0 dxstream
echo    - Uninstall:          build.bat --uninstall
echo ==========================================================

endlocal
exit /b 0

REM ============================================================
REM  Subroutine: build a standalone meson subproject
REM  Usage: call :build_subproject "path\to\project" "display_name"
REM ============================================================
:build_subproject
setlocal EnableDelayedExpansion
set "SUB_DIR=%~1"
set "SUB_NAME=%~2"
set "SUB_BUILD=%SUB_DIR%\builddir"

pushd "%SUB_DIR%" 2>nul || (
    echo [ERROR] Cannot enter %SUB_DIR%
    exit /b 1
)

if exist "%SUB_BUILD%\build.ninja" (
    meson setup "%SUB_BUILD%" --reconfigure %SUB_MESON_OPTS% >nul 2>&1
) else (
    meson setup "%SUB_BUILD%" %SUB_MESON_OPTS% >nul 2>&1
)
if !errorlevel! neq 0 (
    echo [FAIL] %SUB_NAME% - setup
    popd
    endlocal
    exit /b 1
)

meson compile -C "%SUB_BUILD%" >nul 2>&1
if !errorlevel! neq 0 (
    echo [FAIL] %SUB_NAME% - compile
    popd
    endlocal
    exit /b 1
) else (
    echo [ OK ] %SUB_NAME%
)

meson install -C "%SUB_BUILD%" --no-rebuild >nul 2>&1

popd
endlocal
exit /b 0

REM ============================================================
REM  Subroutine: Register environment variables (user-level)
REM ============================================================
:setup_env
echo.
echo [INFO] Registering environment variables...

set "PLUGIN_PATH=%PREFIX%\lib\gstreamer-1.0"
set "RUNTIME_PATH=%PREFIX%\bin"
set "CUSTOMLIB_PATH=%PREFIX%\share\gstdxstream\lib"
set "APPS_PATH=%PREFIX%\share\gstdxstream\bin"

REM -- Set GST_PLUGIN_PATH --
setx GST_PLUGIN_PATH "%PLUGIN_PATH%" >nul 2>&1
if !errorlevel! equ 0 (
    echo   GST_PLUGIN_PATH = %PLUGIN_PATH%
) else (
    echo   [WARN] Failed to set GST_PLUGIN_PATH
)

REM -- Add entries to user PATH (avoid duplicates) --
set "PATHS_TO_ADD=%RUNTIME_PATH%;%CUSTOMLIB_PATH%;%APPS_PATH%;%PLUGIN_PATH%"

powershell -NoProfile -Command ^
    "$paths = '%PATHS_TO_ADD%' -split ';' | Where-Object { $_ -ne '' };" ^
    "$normalize = { param($p) $p.Trim().TrimEnd([char[]]'\/').ToLowerInvariant() };" ^
    "$targetKeys = @{}; foreach ($p in $paths) { $targetKeys[(& $normalize $p)] = $true; }" ^
    "$userPath = [Environment]::GetEnvironmentVariable('Path', 'User');" ^
    "if ($null -eq $userPath) { $userPath = ''; }" ^
    "$parts = $userPath -split ';' | Where-Object { $_ -ne '' };" ^
    "$filtered = New-Object System.Collections.Generic.List[string];" ^
    "$seen = @{};" ^
    "foreach ($entry in $parts) {" ^
    "  $key = & $normalize $entry;" ^
    "  if ($targetKeys.ContainsKey($key)) { continue; }" ^
    "  if (-not $seen.ContainsKey($key)) { $filtered.Add($entry); $seen[$key] = $true; }" ^
    "}" ^
    "$newPath = (@($paths) + @($filtered)) -join ';';" ^
    "if ($newPath -ne $userPath) {" ^
    "  [Environment]::SetEnvironmentVariable('Path', $newPath, 'User');" ^
    "  Write-Host '  PATH entries updated.'" ^
    "} else {" ^
    "  Write-Host '  PATH entries already registered.'" ^
    "}"

echo.
echo [INFO] Environment variables registered. Restart terminal to apply.
exit /b 0

REM ============================================================
REM  Subroutine: Uninstall (remove install/ + env vars + builddirs)
REM ============================================================
:do_uninstall
echo.
echo [INFO] === Uninstalling DX-Stream ===

REM -- Remove install directory --
if exist "%PREFIX%" (
    echo [INFO] Removing %PREFIX%
    rmdir /S /Q "%PREFIX%"
) else (
    echo [INFO] Install directory not found, skipping.
)

REM -- Remove build directories --
if exist "%BUILD_DIR%" (
    echo [INFO] Removing %BUILD_DIR%
    rmdir /S /Q "%BUILD_DIR%"
)
set "DXSTREAM_DIR=%PROJECT_ROOT%\dx_stream"
for /D %%D in ("%DXSTREAM_DIR%\custom_library\postprocess_library\*") do (
    if exist "%%D\builddir" rmdir /S /Q "%%D\builddir"
)
if exist "%DXSTREAM_DIR%\custom_library\message_convert_library\dx_msgconvl\builddir" (
    rmdir /S /Q "%DXSTREAM_DIR%\custom_library\message_convert_library\dx_msgconvl\builddir"
)
for /D %%D in ("%DXSTREAM_DIR%\apps\*") do (
    if exist "%%D\builddir" rmdir /S /Q "%%D\builddir"
)
echo [INFO] Build directories removed.

REM -- Uninstall pydxs and remove venv --
set "VENV_PATH=%PROJECT_ROOT%\venv-dx_stream"
if exist "!VENV_PATH!\Scripts\activate.bat" (
    call "!VENV_PATH!\Scripts\activate.bat"
    python -m pip uninstall -y pydxs >nul 2>&1
    call deactivate 2>nul
    echo [INFO] pydxs uninstalled from venv.
)
if exist "!VENV_PATH!" (
    rmdir /S /Q "!VENV_PATH!"
    echo [INFO] Virtual environment removed: !VENV_PATH!
)
if exist "%PROJECT_ROOT%\bindings\python\pydxs\build" rmdir /S /Q "%PROJECT_ROOT%\bindings\python\pydxs\build"
if exist "%PROJECT_ROOT%\bindings\python\pydxs\dist" rmdir /S /Q "%PROJECT_ROOT%\bindings\python\pydxs\dist"
if exist "%PROJECT_ROOT%\bindings\python\pydxs\src\pydxs.egg-info" rmdir /S /Q "%PROJECT_ROOT%\bindings\python\pydxs\src\pydxs.egg-info"

REM -- Remove GST_PLUGIN_PATH --
reg query "HKCU\Environment" /v GST_PLUGIN_PATH >nul 2>&1
if !errorlevel! equ 0 (
    reg delete "HKCU\Environment" /v GST_PLUGIN_PATH /f >nul 2>&1
    echo [INFO] Removed GST_PLUGIN_PATH
) else (
    echo [INFO] GST_PLUGIN_PATH not set, skipping.
)

REM -- Remove entries from user PATH --
set "PLUGIN_PATH=%PREFIX%\lib\gstreamer-1.0"
set "RUNTIME_PATH=%PREFIX%\bin"
set "CUSTOMLIB_PATH=%PREFIX%\share\gstdxstream\lib"
set "APPS_PATH=%PREFIX%\share\gstdxstream\bin"
set "PATHS_TO_REMOVE=%RUNTIME_PATH%;%CUSTOMLIB_PATH%;%APPS_PATH%;%PLUGIN_PATH%"

powershell -NoProfile -Command ^
    "$removals = '%PATHS_TO_REMOVE%' -split ';' | Where-Object { $_ -ne '' };" ^
    "$normalize = { param($p) $p.Trim().TrimEnd([char[]]'\/').ToLowerInvariant() };" ^
    "$removeKeys = @{}; foreach ($p in $removals) { $removeKeys[(& $normalize $p)] = $true; }" ^
    "$userPath = [Environment]::GetEnvironmentVariable('Path', 'User');" ^
    "if ($null -eq $userPath) { $userPath = ''; }" ^
    "$parts = $userPath -split ';' | Where-Object { $_ -ne '' };" ^
    "$filtered = New-Object System.Collections.Generic.List[string];" ^
    "foreach ($entry in $parts) {" ^
    "  $key = & $normalize $entry;" ^
    "  if (-not $removeKeys.ContainsKey($key)) { $filtered.Add($entry); }" ^
    "}" ^
    "$newPath = ($filtered -join ';');" ^
    "if ($newPath -ne $userPath) {" ^
    "  [Environment]::SetEnvironmentVariable('Path', $newPath, 'User');" ^
    "  Write-Host '[INFO] PATH entries removed.'" ^
    "} else {" ^
    "  Write-Host '[INFO] No PATH entries to remove.'" ^
    "}"

echo.
echo [OK] Uninstall complete. Restart terminal to apply environment changes.
exit /b 0
