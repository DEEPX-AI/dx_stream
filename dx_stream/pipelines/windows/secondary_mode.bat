@echo off
setlocal
title DX-Stream Secondary Mode (Multi-Model Cascade)

if not defined DXSTREAM_ROOT (
    echo [ERROR] DXSTREAM_ROOT not set. Run run_demo.bat or set DXSTREAM_ROOT manually.
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
