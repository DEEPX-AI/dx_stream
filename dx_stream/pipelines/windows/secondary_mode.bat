@echo off
setlocal
title DX-Stream Secondary Mode (Multi-Model Cascade)

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

set "MODEL_PRIMARY=%MODEL_DIR%/yolov5-s_640x640_ppu.dxnn"
set "MODEL_CLASS=%MODEL_DIR%/efficientnet-lite0_256x256.dxnn"
set "MODEL_FACE=%MODEL_DIR%/scrfd-500m_640x640.dxnn"
set "LIB_PRIMARY=%LIB_DIR%/postprocess_ppu.dll"
set "LIB_CLASS=%LIB_DIR%/postprocess_object_class.dll"
set "LIB_FACE=%LIB_DIR%/postprocess_scrfd500m.dll"
set "TRACKER_CONFIG=%CONFIG_DIR%/tracker_config.json"
set "VIDEO=%VIDEO_DIR%/dance-group.mov"

if not "%~1"=="" set "VIDEO=%~1"

echo ============================================================
echo  DX-Stream Secondary Mode (Multi-Model Cascade)
echo ============================================================
echo  Primary  : YoloV5S_PPU (Object Detection)
echo  Secondary: EfficientNet_Lite0 (Classification)
echo             SCRFD500M (Face Detection)
echo  Video    : %VIDEO%
echo ============================================================

gst-launch-1.0 ^
    urisourcebin uri="file:///%VIDEO%" ^
    ! decodebin ^
    ! dxpreprocess preprocess-id=1 resize-width=640 resize-height=640 keep-ratio=true pad-value=114 interval=0 ^
    ! queue ^
    ! dxinfer preprocess-id=1 inference-id=1 model-path="%MODEL_PRIMARY%" ^
    ! queue ^
    ! dxpostprocess inference-id=1 library-file-path="%LIB_PRIMARY%" function-name=YOLOV5S_PPU ^
    ! queue ^
    ! dxtracker config-file-path="%TRACKER_CONFIG%" ^
    ! queue ^
    ! tee name=t ^
    t. ! queue ^
    ! dxpreprocess preprocess-id=2 resize-width=224 resize-height=224 secondary-mode=true interval=5 min-object-width=50 min-object-height=50 keep-ratio=false ^
    ! queue max-size-buffers=1 ^
    ! dxinfer preprocess-id=2 inference-id=2 secondary-mode=true model-path="%MODEL_CLASS%" ^
    ! queue max-size-buffers=1 ^
    ! dxpostprocess inference-id=2 secondary-mode=true library-file-path="%LIB_CLASS%" function-name=PostProcess ^
    ! queue max-size-buffers=1 ^
    ! gather.sink_0 ^
    t. ! queue ^
    ! dxpreprocess preprocess-id=3 resize-width=640 resize-height=640 keep-ratio=true pad-value=114 secondary-mode=true target-class-id=0 min-object-width=50 min-object-height=50 interval=5 ^
    ! queue max-size-buffers=1 ^
    ! dxinfer preprocess-id=3 inference-id=3 secondary-mode=true model-path="%MODEL_FACE%" ^
    ! queue max-size-buffers=1 ^
    ! dxpostprocess inference-id=3 secondary-mode=true library-file-path="%LIB_FACE%" function-name=PostProcess ^
    ! queue max-size-buffers=1 ^
    ! gather.sink_1 ^
    dxgather name=gather ^
    ! queue ^
    ! dxosd ^
    ! queue ^
    ! videoconvert ^
    ! fpsdisplaysink sync=false

endlocal
