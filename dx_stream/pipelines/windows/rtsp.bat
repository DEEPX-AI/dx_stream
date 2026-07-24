@echo off
setlocal
title DX-Stream Multi-Channel Object Detection (RTSP)

if not defined DXSTREAM_ROOT (
    echo [ERROR] DXSTREAM_ROOT not set. Run run_demo.bat or set DXSTREAM_ROOT manually.
    exit /b 1
)

set "BASE_FWD=%DXSTREAM_ROOT:\=/%"
set "MODEL_DIR=%BASE_FWD%/dx_stream/samples/models"
set "LIB_DIR=%BASE_FWD%/install/share/gstdxstream/lib"

set "MODEL=%MODEL_DIR%/yolov5-s_640x640_ppu.dxnn"
set "LIB=%LIB_DIR%/postprocess_ppu.dll"

set "USE_INTERNAL=0"
if "%~1"=="--internal-rtsp" set "USE_INTERNAL=1"

if "%USE_INTERNAL%"=="1" (
    set "RTSP0=rtsp://192.168.30.100:8554/stream2"
    set "RTSP1=rtsp://192.168.30.100:8554/stream3"
    set "RTSP2=rtsp://192.168.30.100:8554/stream4"
    set "RTSP3=rtsp://192.168.30.100:8554/stream5"
) else (
    set "RTSP0=rtsp://210.99.70.120:1935/live/cctv002.stream"
    set "RTSP1=rtsp://210.99.70.120:1935/live/cctv003.stream"
    set "RTSP2=rtsp://210.99.70.120:1935/live/cctv004.stream"
    set "RTSP3=rtsp://210.99.70.120:1935/live/cctv005.stream"
)

echo ============================================================
echo  DX-Stream Multi-Channel Object Detection (RTSP)
echo ============================================================
echo  Output : 1280x720 (2x2 grid, 640x360 each)
echo  Stream0: %RTSP0%
echo  Stream1: %RTSP1%
echo  Stream2: %RTSP2%
echo  Stream3: %RTSP3%
echo ============================================================

gst-launch-1.0 -e ^
    urisourcebin uri="%RTSP0%" ! queue max-size-buffers=2 ! decodebin ! queue max-size-buffers=2 ! in.sink_0 ^
    urisourcebin uri="%RTSP1%" ! queue max-size-buffers=2 ! decodebin ! queue max-size-buffers=2 ! in.sink_1 ^
    urisourcebin uri="%RTSP2%" ! queue max-size-buffers=2 ! decodebin ! queue max-size-buffers=2 ! in.sink_2 ^
    urisourcebin uri="%RTSP3%" ! queue max-size-buffers=2 ! decodebin ! queue max-size-buffers=2 ! in.sink_3 ^
    dxinputselector name=in ^
    ! dxpreprocess preprocess-id=1 resize-width=640 resize-height=640 keep-ratio=true pad-value=114 ! queue max-size-buffers=2 ^
    ! dxinfer preprocess-id=1 inference-id=1 model-path="%MODEL%" ! queue max-size-buffers=2 ^
    ! dxpostprocess inference-id=1 library-file-path="%LIB%" function-name=YOLOV5S_PPU ! queue max-size-buffers=2 ^
    ! dxosd ^
    ! dxoutputselector name=out ^
    out.src_0 ! queue max-size-buffers=2 ! dxscale width=640 height=360 ! queue max-size-buffers=2 ! comp.sink_0 ^
    out.src_1 ! queue max-size-buffers=2 ! dxscale width=640 height=360 ! queue max-size-buffers=2 ! comp.sink_1 ^
    out.src_2 ! queue max-size-buffers=2 ! dxscale width=640 height=360 ! queue max-size-buffers=2 ! comp.sink_2 ^
    out.src_3 ! queue max-size-buffers=2 ! dxscale width=640 height=360 ! queue max-size-buffers=2 ! comp.sink_3 ^
    compositor name=comp sink_0::xpos=0 sink_0::ypos=0 sink_1::xpos=640 sink_1::ypos=0 sink_2::xpos=0 sink_2::ypos=360 sink_3::xpos=640 sink_3::ypos=360 ^
    ! videoconvert ^
    ! fpsdisplaysink sync=false

endlocal
