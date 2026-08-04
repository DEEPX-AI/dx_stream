@echo off
setlocal
title DX-Stream Broker Pipeline (Kafka)

if not defined DXSTREAM_ROOT (
    pushd "%~dp0..\..\..\"
    set "DXSTREAM_ROOT=%CD%"
    popd
)

set "GST_PLUGIN_PATH=%DXSTREAM_ROOT%\install\lib\gstreamer-1.0"
set "GST_REGISTRY=%DXSTREAM_ROOT%\install\gst-registry.bin"
set "PATH=%DXSTREAM_ROOT%\install\bin;%DXSTREAM_ROOT%\install\share\gstdxstream\lib;%DXSTREAM_ROOT%\install\share\gstdxstream\bin;%DXSTREAM_ROOT%\install\lib\gstreamer-1.0;%PATH%"

if defined GSTREAMER_1_0_ROOT_MSVC_X86_64 (
    set "PATH=%GSTREAMER_1_0_ROOT_MSVC_X86_64%\bin;%PATH%"
) else if exist "C:\Program Files\gstreamer\1.0\msvc_x86_64\bin" (
    set "PATH=C:\Program Files\gstreamer\1.0\msvc_x86_64\bin;%PATH%"
)

if not exist "%DXSTREAM_ROOT%\install\bin\gstdxstream.dll" (
    echo [ERROR] DX-Stream plugin not found at "%DXSTREAM_ROOT%\install\bin\gstdxstream.dll".
    echo         Run 'build.bat' first.
    exit /b 1
)

set "BASE_FWD=%DXSTREAM_ROOT:\=/%"
set "MODEL_DIR=%BASE_FWD%/dx_stream/samples/models"
set "VIDEO_DIR=%BASE_FWD%/dx_stream/samples/videos"
set "LIB_DIR=%BASE_FWD%/install/share/gstdxstream/lib"
set "CONFIG_DIR=%BASE_FWD%/dx_stream/configs"

set "MODEL=%MODEL_DIR%/yolov5-s_640x640_ppu.dxnn"
set "LIB=%LIB_DIR%/postprocess_ppu.dll"
set "MSGCONV_LIB=%LIB_DIR%/dx_msgconvl.dll"
set "BROKER_CONFIG=%CONFIG_DIR%/broker_kafka.cfg"
set "BROKER_CONN=localhost:9092"
set "TOPIC=test"
set "VIDEO=%VIDEO_DIR%/blackbox-city-road.mp4"

if not "%~1"=="" set "VIDEO=%~1"

echo ============================================================
echo  DX-Stream Broker Pipeline (Kafka)
echo ============================================================
echo  Model  : %MODEL%
echo  Video  : %VIDEO%
echo  Broker : kafka @ %BROKER_CONN%  topic=%TOPIC%
echo ============================================================

gst-launch-1.0 ^
    filesrc location="%VIDEO%" ^
    ! decodebin ^
    ! videoconvert ! "video/x-raw,format=NV12" ^
    ! dxpreprocess preprocess-id=1 resize-width=640 resize-height=640 keep-ratio=true pad-value=114 ^
    ! queue max-size-buffers=1 ^
    ! dxinfer preprocess-id=1 inference-id=1 model-path="%MODEL%" ^
    ! queue max-size-buffers=1 ^
    ! dxpostprocess inference-id=1 library-file-path="%LIB%" function-name=YOLOV5S_PPU ^
    ! queue max-size-buffers=1 ^
    ! dxosd ^
    ! queue max-size-buffers=1 ^
    ! dxmsgconv library-file-path="%MSGCONV_LIB%" include-frame=true ^
    ! queue max-size-buffers=1 ^
    ! dxmsgbroker broker-name=kafka conn-info="%BROKER_CONN%" topic="%TOPIC%" config="%BROKER_CONFIG%"

endlocal
