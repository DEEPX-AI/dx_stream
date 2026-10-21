@echo off
chcp 65001 >nul
REM pydxs Python binding test suite (Windows)
REM Usage:  run_pydxs.bat
REM Exit code: 0=PASS, 1=FAIL, 77=contains optional SKIP
setlocal EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
set "PROJECT_ROOT=%SCRIPT_DIR%\.."
for %%I in ("%PROJECT_ROOT%") do set "PROJECT_ROOT=%%~fI"

set "VENV_PATH=%PROJECT_ROOT%\venv-dx_stream"
set "PYDXS_TEST=%SCRIPT_DIR%\base\pydxs\test_pydxs.py"
set "LOG_DIR=%SCRIPT_DIR%\_logs"
if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"
set PASS=0
set SKIP=0

echo ===== Test Group: pydxs =====
echo.
echo   [INFO] PyGObject (gi) availability determines which tests run:
echo   [INFO]   CPY1 (import^), CPY3 (object meta^)    : always executed
echo   [INFO]   CPY2 (frame meta^), CPY4 (numpy^), CPY5 (user meta^), CPY6 (depth^) : requires gi
echo   [INFO]   gi requires compatible Python and GStreamer builds.
echo   [INFO]   Unavailable gi = CPY2/CPY4/CPY5/CPY6 explicitly skipped.
echo.

REM Check test file exists
if not exist "%PYDXS_TEST%" (
    echo   [FAIL] test_pydxs.py not found
    echo   -- summary: PASS=0 FAIL=1 SKIP=0 --
    echo.
    exit /b 1
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
call "%PROJECT_ROOT%\scripts\setup_gstreamer.bat" runtime
if errorlevel 1 exit /b 1

if exist "%INSTALL_PREFIX%\lib\gstreamer-1.0" (
    set "GST_PLUGIN_PATH=%INSTALL_PREFIX%\lib\gstreamer-1.0"
    set "GST_REGISTRY=%INSTALL_PREFIX%\gst-registry.bin"
    set "PATH=%INSTALL_PREFIX%\bin;%INSTALL_PREFIX%\lib\gstreamer-1.0;!PATH!"
)

REM Make GStreamer runtime DLLs and GI typelibs available
if exist "!GST_ROOT!\bin" set "PATH=!GST_ROOT!\bin;!PATH!"
if exist "!GST_ROOT!\lib\girepository-1.0" set "GI_TYPELIB_PATH=!GST_ROOT!\lib\girepository-1.0;!GI_TYPELIB_PATH!"

REM DEEPX runtime (dxrt.dll) — dependency of gstdxstream.dll
if defined DEEPX_SDK_DIR if exist "!DEEPX_SDK_DIR!\bin" set "PATH=!DEEPX_SDK_DIR!\bin;!PATH!"
if exist "C:\Program Files\DEEPX\DXNN\runtime" set "PATH=C:\Program Files\DEEPX\DXNN\runtime;!PATH!"

REM Run tests
python "%PYDXS_TEST%" > "%LOG_DIR%\pydxs.log" 2>&1
if !errorlevel! equ 0 (
    findstr /B /C:"SKIP:" "%LOG_DIR%\pydxs.log" >nul
    if !errorlevel! equ 0 (
        echo   [SKIP] pydxs ^(contains optional skipped checks^)
        findstr /B /C:"SKIP:" "%LOG_DIR%\pydxs.log"
        set SKIP=1
    ) else (
        echo   [PASS] pydxs
        set PASS=1
    )
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

REM Check cold imports and DLL discovery without test-side path registration.
python -m unittest discover -s "%SCRIPT_DIR%\windows" -p test_pydxs_loader.py -v > "%LOG_DIR%\pydxs_loader.log" 2>&1
if !errorlevel! equ 0 (
    echo   [PASS] pydxs_loader
    set /a PASS+=1
    echo   -- summary: PASS=!PASS! FAIL=0 SKIP=!SKIP! --
) else (
    echo   [FAIL] pydxs_loader
    type "%LOG_DIR%\pydxs_loader.log"
    echo   -- summary: PASS=!PASS! FAIL=1 SKIP=!SKIP! --
    call deactivate 2>nul
    endlocal
    exit /b 1
)
echo.

set RESULT=0
if !SKIP! neq 0 set RESULT=77
call deactivate 2>nul
endlocal & exit /b %RESULT%
