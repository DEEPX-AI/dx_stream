## PR 164
### 1. Changed
### 2. Fixed
- Use GstVideoInfo stride/offset for NV12 CPU input instead of RGA heuristic(dxconvert/dxscale)
### 3. Added
- Auto quality test in dxscale, dxconvert unit test added
## PR 163
### 1. Changed
- **Preprocessing Architecture**: Introduced IVideoTransformKernel abstraction layer with platform-specific backends (RGA, libyuv, V3 DSP) and VideoTransformFactory for automatic backend selection [DSA1-555](https://deepx.atlassian.net/browse/DSA1-555)
### 2. Fixed
### 3. Added
- **DxScale Element**: New GstBaseTransform element for HW-accelerated video scaling (NV12/I420/RGB/BGR) [DSA1-555](https://deepx.atlassian.net/browse/DSA1-555)
- **DxConvert Element**: New GstBaseTransform element for color format conversion with full 4×4 format matrix support [DSA1-555](https://deepx.atlassian.net/browse/DSA1-555)
## PR 161
### 1. Changed
- **DXOSD Refactor**: Removed unnecessary color conversion, resize, and intermediate memory allocation from the dxosd plugin to simplify processing flow [DSA1-547](https://deepx.atlassian.net/browse/DSA1-547)
- **YUV In-Place Drawing**: Added support for rendering overlays directly onto YUV input buffers, eliminating format conversion overhead [DSA1-547](https://deepx.atlassian.net/browse/DSA1-547)
### 2. Fixed
### 3. Added
## PR 159 NOTHING NEW
