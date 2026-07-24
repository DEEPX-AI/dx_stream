@echo off
chcp 65001 >nul
REM pydxs Python binding test suite (Windows)
REM Usage:  run_pydxs.bat
REM Exit code: 0=PASS, 1=FAIL
setlocal EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
set "PROJECT_ROOT=%SCRIPT_DIR%\.."
for %%I in ("%PROJECT_ROOT%") do set "PROJECT_ROOT=%%~fI"

set "VENV_PATH=%PROJECT_ROOT%\venv-dx_stream"
set "PYDXS_TEST=%SCRIPT_DIR%\base\pydxs\test_pydxs.py"
set "LOG_DIR=%SCRIPT_DIR%\_logs"
if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"

echo ===== Test Group: pydxs =====
echo.
echo   [INFO] PyGObject (gi) availability determines which tests run:
echo   [INFO]   CPY1 (import^), CPY3 (object meta^)    : always executed
echo   [INFO]   CPY2 (frame meta^), CPY4 (numpy^), CPY5 (user meta^) : requires gi
echo   [INFO]   GStreamer MSI (1.26.1) bundles gi for Python 3.13 only.
echo   [INFO]   Python version mismatch = CPY2/CPY4/CPY5 auto-skipped.
echo.

REM Check test file exists
if not exist "%PYDXS_TEST%" (
    echo   [SKIP] test_pydxs.py not found
    echo   -- summary: PASS=0 FAIL=0 ^(skipped^) --
    echo.
    exit /b 0
)

REM Activate venv if available
if exist "!VENV_PATH!\Scripts\activate.bat" (
    call "!VENV_PATH!\Scripts\activate.bat"
) else (
    echo   [WARN] venv not found: !VENV_PATH!
)

REM Ensure GStreamer plugin and DLLs are findable
set "INSTALL_PREFIX=%PROJECT_ROOT%\install"
set "DX_STREAM_ROOT=%PROJECT_ROOT%"
set "PYTHONIOENCODING=utf-8"

REM GStreamer MSVC root (for runtime DLLs + GI typelibs)
if defined GSTREAMER_1_0_ROOT_MSVC_X86_64 (
    set "GST_ROOT=%GSTREAMER_1_0_ROOT_MSVC_X86_64%"
) else (
    set "GST_ROOT=C:\Program Files\gstreamer\1.0\msvc_x86_64"
)

if exist "%INSTALL_PREFIX%\lib\gstreamer-1.0" (
    set "GST_PLUGIN_PATH=%INSTALL_PREFIX%\lib\gstreamer-1.0;!GST_PLUGIN_PATH!"
    set "PATH=%INSTALL_PREFIX%\bin;%INSTALL_PREFIX%\lib\gstreamer-1.0;!PATH!"
)

REM Make GStreamer runtime DLLs and GI typelibs available
if exist "!GST_ROOT!\bin" set "PATH=!GST_ROOT!\bin;!PATH!"
if exist "!GST_ROOT!\lib\girepository-1.0" set "GI_TYPELIB_PATH=!GST_ROOT!\lib\girepository-1.0;!GI_TYPELIB_PATH!"

REM DEEPX runtime (dxrt.dll) — dependency of gstdxstream.dll
if exist "C:\Program Files\DEEPX\DXNN\runtime" set "PATH=C:\Program Files\DEEPX\DXNN\runtime;!PATH!"

REM Run tests
python "%PYDXS_TEST%" > "%LOG_DIR%\pydxs.log" 2>&1
if !errorlevel! equ 0 (
    echo   [PASS] pydxs
    echo   -- summary: PASS=1 FAIL=0 --
) else (
    echo   [FAIL] pydxs
    if exist "%LOG_DIR%\pydxs.log" (
        echo   ---- pydxs.log ----
        type "%LOG_DIR%\pydxs.log"
    )
    echo   -- summary: PASS=0 FAIL=1 --
    call deactivate 2>nul
    endlocal
    exit /b 1
)
echo.

call deactivate 2>nul
endlocal
exit /b 0
