@echo off
REM ============================================================
REM  DX-Stream Windows Setup Script
REM  Downloads sample models and videos for running examples.
REM  Usage:
REM    setup.bat                       -> download all models + videos
REM    setup.bat --model=yolov5-s_640x640.dxnn  -> download single model only
REM    setup.bat --force               -> re-download even if exists
REM ============================================================

setlocal EnableDelayedExpansion

set "PROJECT_ROOT=%~dp0"
if "%PROJECT_ROOT:~-1%"=="\" set "PROJECT_ROOT=%PROJECT_ROOT:~0,-1%"

set "MODEL_LIST=%PROJECT_ROOT%\model_list.json"
set "MODEL_DIR=%PROJECT_ROOT%\dx_stream\samples\models"
set "VIDEO_DIR=%PROJECT_ROOT%\dx_stream\samples\videos"
set "DOWNLOAD_DIR=%PROJECT_ROOT%\download"
set "BASE_URL=https://sdk.deepx.ai/modelzoo/dxnn"
set "VIDEO_URL=https://sdk.deepx.ai/res/video/sample_videos.tar.gz"

set "FORCE=0"
set "SINGLE_MODEL="

:parse_args
if "%~1"=="" goto args_done
if /I "%~1"=="--force" (set "FORCE=1" & shift & goto parse_args)
if /I "%~1"=="--help" goto show_help
set "ARG=%~1"
if "!ARG:~0,8!"=="--model=" (
    set "SINGLE_MODEL=!ARG:~8!"
    shift & goto parse_args
)
echo [ERROR] Unknown option: %~1
exit /b 1
:args_done

echo ============================================================
echo  DX-Stream Setup (Models ^& Videos)
echo ============================================================
echo.

REM ---- Check prerequisites ----
where curl >nul 2>nul
if errorlevel 1 (
    echo [ERROR] curl not found. Windows 10+ should include curl by default.
    exit /b 1
)

if not exist "%MODEL_LIST%" (
    echo [ERROR] model_list.json not found at %MODEL_LIST%
    exit /b 1
)

REM ---- Parse model_list.json ----
for /f "delims=" %%V in ('powershell -NoProfile -Command "(Get-Content '%MODEL_LIST%' | ConvertFrom-Json).version"') do set "MODEL_VERSION=%%V"
echo   Model version: %MODEL_VERSION%
echo.

REM ---- Create output directories ----
if not exist "%MODEL_DIR%" mkdir "%MODEL_DIR%"

REM ============================================================
REM  Phase 1: Download models
REM ============================================================
echo [Phase 1] Downloading models...
echo.

set "MODEL_OK=0"
set "MODEL_SKIP=0"
set "MODEL_FAIL=0"

if defined SINGLE_MODEL (
    call :download_model "!SINGLE_MODEL!"
) else (
    for /f "delims=" %%M in ('powershell -NoProfile -Command "(Get-Content '%MODEL_LIST%' | ConvertFrom-Json).models | ForEach-Object { $_ }"') do (
        call :download_model "%%M"
    )
)

echo.
echo   Result: !MODEL_OK! downloaded, !MODEL_SKIP! skipped, !MODEL_FAIL! failed
echo.

REM ============================================================
REM  Phase 2: Download sample videos
REM ============================================================
if defined SINGLE_MODEL goto done

echo [Phase 2] Downloading sample videos...
echo.

if exist "%VIDEO_DIR%" if "!FORCE!"=="0" (
    echo   [SKIP] Videos already exist: %VIDEO_DIR%
    goto done
)

if not exist "%DOWNLOAD_DIR%" mkdir "%DOWNLOAD_DIR%"

set "VIDEO_TAR=%DOWNLOAD_DIR%\sample_videos.tar.gz"

if exist "%VIDEO_TAR%" if "!FORCE!"=="0" (
    echo   [SKIP] Archive already downloaded: %VIDEO_TAR%
    goto extract_video
)

echo   Downloading sample_videos.tar.gz ...
curl -fSL -o "%VIDEO_TAR%" "%VIDEO_URL%"
if errorlevel 1 (
    echo   [FAIL] Video download failed.
    del /Q "%VIDEO_TAR%" 2>nul
    exit /b 1
)
echo   [OK] Downloaded sample_videos.tar.gz

:extract_video
echo   Extracting to %VIDEO_DIR% ...
if exist "%VIDEO_DIR%" rmdir /S /Q "%VIDEO_DIR%"
mkdir "%VIDEO_DIR%"
tar -xzf "%VIDEO_TAR%" -C "%VIDEO_DIR%" --strip-components=1 2>nul
if errorlevel 1 (
    tar -xzf "%VIDEO_TAR%" -C "%VIDEO_DIR%"
    if errorlevel 1 (
        echo   [FAIL] Video extraction failed.
        exit /b 1
    )
)
echo   [OK] Videos extracted

:done
echo.
echo ============================================================
echo  Setup complete.
echo    Models: %MODEL_DIR%
echo    Videos: %VIDEO_DIR%
echo ============================================================
if !MODEL_FAIL! gtr 0 (
    echo  WARNING: !MODEL_FAIL! model download(s) failed.
)
endlocal & exit /b %MODEL_FAIL%

REM ============================================================
REM  Subroutine: download a single model
REM ============================================================
:download_model
set "MODEL_NAME=%~1"
set "MODEL_PATH=%MODEL_DIR%\%MODEL_NAME%"
set "MODEL_URL=%BASE_URL%/%MODEL_VERSION%/%MODEL_NAME%"

if exist "%MODEL_PATH%" if "!FORCE!"=="0" (
    echo   [SKIP] %MODEL_NAME%
    set /a MODEL_SKIP+=1
    goto :eof
)

echo   Downloading %MODEL_NAME% ...
curl -fSL -o "%MODEL_PATH%" "%MODEL_URL%"
if errorlevel 1 (
    echo   [FAIL] %MODEL_NAME%
    del /Q "%MODEL_PATH%" 2>nul
    set /a MODEL_FAIL+=1
    goto :eof
)
echo   [OK] %MODEL_NAME%
set /a MODEL_OK+=1
goto :eof

REM ============================================================
:show_help
echo Usage: setup.bat [OPTIONS]
echo Options:
echo   --model=^<name^>   Download only the specified model (must exist in model_list.json)
echo   --force           Force re-download even if files already exist
echo   --help            Show this help message
echo.
echo If --model is not given, all models and videos are downloaded.
exit /b 0
