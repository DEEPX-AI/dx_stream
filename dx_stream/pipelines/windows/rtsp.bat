@echo off
setlocal
title DX-Stream Multi-Channel Object Detection (RTSP)

rem Delayed expansion stays off so '!' in the pipeline and URLs is kept as-is.
set "SCRIPT_NAME=%~nx0"
set "SRC="
set "OUT="
set "COMP="
set /a CH=0

:parse
if "%~1"=="" goto parsed
if "%~1"=="-h" goto help
if "%~1"=="--help" goto help
if not "%~1"=="--rtsp-url" (
    echo [ERROR] Unknown option or argument: "%~1" 1>&2
    goto usage_error
)
set "URL=%~2"
if not defined URL (
    echo [ERROR] --rtsp-url requires a value 1>&2
    goto usage_error
)
rem URLs are not printed because they may contain credentials.
if not "%URL: =%"=="%URL%" (
    echo [ERROR] RTSP URL must not contain whitespace 1>&2
    goto usage_error
)
if "%URL:~0,7%"=="rtsp://" goto url_ok
if "%URL:~0,8%"=="rtsps://" goto url_ok
echo [ERROR] RTSP URL must start with rtsp:// or rtsps:// 1>&2
goto usage_error
:url_ok
set SRC=%SRC% urisourcebin uri="%URL%" ! queue max-size-buffers=2 ! decodebin ! queue max-size-buffers=2 ! in.sink_%CH%
set /a CH+=1
shift /1
shift /1
goto parse

:help
call :usage
exit /b 0

:usage_error
call :usage 1>&2
exit /b 1

:usage
echo Usage: %SCRIPT_NAME% --rtsp-url URL [--rtsp-url URL ...]
echo.
echo Multi-channel object detection (YOLOv5s PPU) on RTSP streams.
echo One channel is created per --rtsp-url, in the given order.
echo.
echo Options:
echo   --rtsp-url URL   RTSP stream URL (rtsp:// or rtsps://). Repeat for more channels.
echo                    Wrap each URL in double quotes.
echo   -h, --help       Show this help and exit.
echo.
echo Example:
echo   %SCRIPT_NAME% --rtsp-url "rtsp://127.0.0.1:8554/stream1" --rtsp-url "rtsp://127.0.0.1:8554/stream2"
exit /b 0

:parsed
if %CH% EQU 0 (
    echo [ERROR] At least one --rtsp-url is required 1>&2
    goto usage_error
)

if not defined DXSTREAM_ROOT (
    pushd "%~dp0..\..\..\"
    set "DXSTREAM_ROOT=%CD%"
    popd
)

set "GST_PLUGIN_PATH=%DXSTREAM_ROOT%\install\lib\gstreamer-1.0"
set "GST_REGISTRY=%DXSTREAM_ROOT%\install\gst-registry.bin"
set "PATH=%DXSTREAM_ROOT%\install\bin;%DXSTREAM_ROOT%\install\share\gstdxstream\lib;%DXSTREAM_ROOT%\install\share\gstdxstream\bin;%DXSTREAM_ROOT%\install\lib\gstreamer-1.0;%PATH%"

call "%DXSTREAM_ROOT%\scripts\setup_gstreamer.bat" runtime
if errorlevel 1 exit /b 1

if not exist "%DXSTREAM_ROOT%\install\bin\gstdxstream.dll" (
    echo [ERROR] DX-Stream plugin not found at "%DXSTREAM_ROOT%\install\bin\gstdxstream.dll".
    echo         Run 'build.bat' first.
    exit /b 1
)

set "BASE_FWD=%DXSTREAM_ROOT:\=/%"
set "MODEL_DIR=%BASE_FWD%/dx_stream/samples/models"
set "LIB_DIR=%BASE_FWD%/install/share/gstdxstream/lib"

set "MODEL=%MODEL_DIR%/yolov5-s_640x640_ppu.dxnn"
set "LIB=%LIB_DIR%/postprocess_ppu.dll"

set /a COLS=1
:grid
set /a SQ=COLS*COLS
if %SQ% LSS %CH% (
    set /a COLS+=1
    goto grid
)
set /a ROWS=(CH+COLS-1)/COLS
set /a SW=1280/COLS
set /a SH=720/ROWS

set /a I=0
:layout
if %I% GEQ %CH% goto launch
set /a X=(I%%COLS)*SW
set /a Y=(I/COLS)*SH
echo   Adding pipeline: [channel=%I%]
set OUT=%OUT% out.src_%I% ! queue max-size-buffers=2 ! dxscale width=%SW% height=%SH% ! queue max-size-buffers=2 ! comp.sink_%I%
set COMP=%COMP% sink_%I%::xpos=%X% sink_%I%::ypos=%Y%
set /a I+=1
goto layout

:launch
echo ============================================================
echo  DX-Stream Multi-Channel Object Detection (RTSP)
echo ============================================================
echo  Channels: %CH%  Grid: %ROWS% rows x %COLS% columns
echo  Output  : 1280x720 (%SW%x%SH% each)
echo ============================================================
echo [WARN] If the pipeline cannot keep up with the RTSP source rate, frames may be dropped.

REM -q hides progress messages, which would print URL credentials; errors are still reported.
gst-launch-1.0 -q -e %SRC% ^
    dxinputselector name=in ^
    ! dxpreprocess preprocess-id=1 resize-width=640 resize-height=640 keep-ratio=true pad-value=114 ! queue max-size-buffers=2 ^
    ! dxinfer preprocess-id=1 inference-id=1 model-path="%MODEL%" ! queue max-size-buffers=2 ^
    ! dxpostprocess inference-id=1 library-file-path="%LIB%" function-name=YOLOV5S_PPU ! queue max-size-buffers=2 ^
    ! dxosd ^
    ! dxoutputselector name=out ^
    %OUT% ^
    compositor name=comp %COMP% ^
    ! videoconvert ^
    ! fpsdisplaysink sync=false
set "RC=%ERRORLEVEL%"

endlocal & exit /b %RC%
