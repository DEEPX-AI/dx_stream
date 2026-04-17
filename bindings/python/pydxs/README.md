# pydxs

Python bindings for DX-Stream metadata API.

## Overview

`pydxs` provides Python bindings to access and manipulate DX-Stream metadata (`DXFrameMeta`, `DXObjectMeta`, `DXUserMeta`) from GStreamer pipelines. This allows Python applications to:
- Create and manage frame metadata
- Create and add object metadata to frames
- Add custom user metadata to frames and objects
- Access frame-level semantic segmentation maps and object-level ROI-local masks
- Read and iterate through metadata in GStreamer pipelines

## Metadata Layout Notes

Current `pydxs` bindings follow the native metadata headers in `gst-dxstream-plugin/metadata`.

- `DXFrameMeta` includes `seg_data`, `seg_width`, `seg_height`, and `seg_format` for frame-level semantic segmentation.
- `DXObjectMeta` includes `seg_data`, `seg_width`, `seg_height`, and `seg_format` for object-level segmentation.
- `DXObjectMeta.seg_data` stores an ROI-local binary mask aligned to `obj_meta.box`.
- `DXFrameMeta.seg_data` stores a full-frame semantic class map.
- In Python, both frame and object `seg_data` values are exposed as `bytes`.

## Requirements

- **Python:** 3.6 or higher
- **Build System:** setuptools + pybind11 (automatically installed)
- **Dependencies:** GStreamer 1.0, DX-Stream plugin

## Installation

```bash
# From the project root directory

# 1. Install dependencies
./install.sh

# 2. Build DX-Stream (automatically installs pydxs into venv-dx_stream)
./build.sh
```

**What happens:**
- [`./install.sh`](../../../install.sh): Installs GStreamer, OpenCV, python3-gi, and other dependencies
- [`./build.sh`](../../../build.sh):
    - Builds DX-Stream with setuptools
    - Creates a dedicated virtual environment at `PROJECT_ROOT/venv-dx_stream`
    - Uses `python3 -m venv --system-site-packages` so the venv can access system-installed `python3-gi`
    - Installs `pydxs` into this `venv-dx_stream` environment
    - **Automatically sets rpath** so `libgstdxstream.so` is found at runtime (no `LD_LIBRARY_PATH` needed)

**Supported Python Versions:**
- Python 3.6-3.7: Legacy mode with setuptools + pybind11
- Python 3.8+: Modern mode with setuptools + pybind11
- Compatible with Ubuntu 18.04 (Python 3.6) and newer

**Why a dedicated virtual environment?**
- DX-Stream Python apps rely on the GStreamer Python bindings (`gi`, from the `python3-gi` package), which are installed into the **system** Python via your package manager (for example, `apt-get install python3-gi`).
- To use `gi` from a virtual environment, that venv must (1) include system site-packages and (2) be created with the **same Python version** as the system Python.
- To avoid version and environment mismatches, `build.sh` automatically creates and reuses a dedicated virtual environment at `PROJECT_ROOT/venv-dx_stream` using `python3 -m venv --system-site-packages` and installs `pydxs` there.

### Verification

Verify the installation:

```bash
# 1. Activate the virtual environment
source venv-dx_stream/bin/activate

# 2. Run checks in the activated environment
# Check GStreamer Python bindings
python3 -c "import gi; gi.require_version('Gst', '1.0'); from gi.repository import Gst; print('GStreamer OK')"

# Check pydxs
python3 -c "import pydxs; print('pydxs OK')"
```

## Quick Start

### 1. Run Basic Tests

First, verify your installation with automated tests:

```bash
# From the project root directory
cd test/test_pydxs
./run_tests.sh
```

This validates:
- Frame metadata creation
- Object metadata creation and addition
- User metadata operations
- Data consistency across pipeline

**What it does**: Runs automated tests using `videotestsrc` (no video file needed). Tests metadata creation, reading, and validation. [`run_tests.sh`](../../../test/test_pydxs/run_tests.sh) automatically activates `PROJECT_ROOT/venv-dx_stream` (if it exists) before running the Python test.

See [`test/test_pydxs/test_usermeta.py`](../../../test/test_pydxs/test_usermeta.py) for the test implementation.

### 2. Run Full Application Example

Try the complete user metadata pipeline example:

```bash
# From the project root directory

# 1. Activate the virtual environment
source venv-dx_stream/bin/activate

# 2. Run example
python3 dx_stream/apps/usermeta/usermeta_app.py dx_stream/samples/videos/dogs.mp4
```

**What it does**: Demonstrates a real-world pipeline that:
- Creates frame and object metadata from scratch
- Adds multiple types of custom user metadata
- Reads and displays all metadata
- Shows metadata lifecycle in action

See [`dx_stream/apps/usermeta/usermeta_app.py`](../../../dx_stream/apps/usermeta/usermeta_app.py) for the full implementation.


## API Reference

### Working with Frame Metadata

**Create frame metadata** - `writable_buffer(probe_info_address)` (recommended)

Context manager that ensures buffer is writable and returns frame metadata (creates if doesn't exist).

```python
def probe_callback(pad, info):
    with pydxs.writable_buffer(hash(info)) as frame_meta:
        # frame_meta is guaranteed to exist and writable
        # Modify frame properties or add user metadata
        frame_meta.dx_add_user_meta_to_frame(data, type_id)
    return Gst.PadProbeReturn.OK
```

**Create frame metadata** - `dx_create_frame_meta(buffer_address)` (direct)

Directly creates new frame metadata and attaches to buffer.

```python
buffer = pydxs.dx_create_frame_meta(hash(buffer))
```

**Get frame metadata** - `dx_get_frame_meta(buffer_address)`

Retrieves existing frame metadata from buffer (returns `None` if doesn't exist).

```python
frame_meta = pydxs.dx_get_frame_meta(hash(buffer))
if frame_meta:
    # Access frame properties
    print(f"Frame: {frame_meta.width}x{frame_meta.height}")
    print(f"Frame seg format: {frame_meta.seg_format}")
    # Iterate through objects
    for obj_meta in frame_meta:
        print(f"  Object: {obj_meta.label}")
```

### Working with Object Metadata

**Create object metadata** - `dx_acquire_obj_meta_from_pool()`

Acquires object metadata from pool.

```python
obj_meta = pydxs.dx_acquire_obj_meta_from_pool()
```

**Set object properties**

Modify object metadata properties directly.

```python
obj_meta.label = 0
obj_meta.label_name = "dog"
obj_meta.confidence = 0.95
obj_meta.box = [x1, y1, x2, y2]
obj_meta.track_id = 1001
obj_meta.seg_data = bytes(mask_bytes)
obj_meta.seg_width = roi_width
obj_meta.seg_height = roi_height
```

**Add object to frame** - `dx_add_obj_meta_to_frame(frame_meta, obj_meta)`

Adds object metadata to frame.

```python
pydxs.dx_add_obj_meta_to_frame(frame_meta, obj_meta)
```

**Remove object from frame** - `dx_remove_obj_meta_from_frame(frame_meta, obj_meta)`

Removes object metadata from frame.

```python
pydxs.dx_remove_obj_meta_from_frame(frame_meta, obj_meta)
```

**Iterate through objects**

Access all objects in frame metadata.

```python
for obj_meta in frame_meta:
    print(f"Object: label={obj_meta.label}, confidence={obj_meta.confidence}")
    print(f"  Box: {obj_meta.box}")
    print(f"  Track ID: {obj_meta.track_id}")
```

### Working with Segmentation Metadata

`DXFrameMeta` and `DXObjectMeta` both expose segmentation-related fields, but they store different kinds of data:

- `frame_meta.seg_data`: full-frame semantic class map
- `frame_meta.seg_width`, `frame_meta.seg_height`: full-frame segmentation dimensions
- `frame_meta.seg_format`: `"full-frame-class-map"` or `"none"`
- `obj_meta.seg_data`: ROI-local binary mask stored as bytes
- `obj_meta.seg_width`, `obj_meta.seg_height`: ROI-local mask dimensions
- `obj_meta.seg_format`: `"roi-binary-mask"` or `"none"`
- `obj_meta.box`: placement of the ROI-local mask in frame coordinates

```python
if frame_meta.seg_format == "full-frame-class-map":
    print(
        f"Frame segmentation: {frame_meta.seg_width}x{frame_meta.seg_height}, "
        f"{len(frame_meta.seg_data)} bytes"
    )

for obj_meta in frame_meta:
    if obj_meta.seg_format == "roi-binary-mask":
        x1, y1, x2, y2 = obj_meta.box
        print(
            f"Object mask: {obj_meta.seg_width}x{obj_meta.seg_height}, "
            f"{len(obj_meta.seg_data)} bytes, box={(x1, y1, x2, y2)}"
        )
```

Any object-level segmentation consumer should interpret `obj_meta.seg_data` as an ROI-local mask and use `obj_meta.box` to place it back on the frame.

### Working with User Metadata

User metadata allows attaching arbitrary Python objects to frames or objects.

**Attach to frame** - `frame_meta.dx_add_user_meta_to_frame(data, type_id)`

Attaches Python object as user metadata to frame.

```python
custom_data = {"scene": "outdoor", "brightness": 0.8}
frame_meta.dx_add_user_meta_to_frame(custom_data, type_id=1001)
```

**Attach to object** - `obj_meta.dx_add_user_meta_to_obj(data, type_id)`

Attaches Python object as user metadata to object.

```python
detection_info = {"quality": 0.95, "verified": True}
obj_meta.dx_add_user_meta_to_obj(detection_info, type_id=2001)
```

**Retrieve from frame** - `frame_meta.dx_get_frame_user_metas()`

Gets list of user metadata attached to frame.

```python
user_metas = frame_meta.dx_get_frame_user_metas()
for user_meta in user_metas:
    if user_meta.type == 1001:
        data = user_meta.get_data()
        print(f"Custom data: {data}")
```

**Retrieve from object** - `obj_meta.dx_get_object_user_metas()`

Gets list of user metadata attached to object.

```python
user_metas = obj_meta.dx_get_object_user_metas()
for user_meta in user_metas:
    if user_meta.type == 2001:
        data = user_meta.get_data()
        print(f"Detection info: {data}")
```

**Access user metadata properties**

```python
# Get type ID
type_id = user_meta.type

# Get stored Python object
data = user_meta.get_data()
```

---

For complete examples, see:
- [`test/test_pydxs/test_usermeta.py`](../../../test/test_pydxs/test_usermeta.py) - Basic patterns
- [`dx_stream/apps/usermeta/usermeta_app.py`](../../../dx_stream/apps/usermeta/usermeta_app.py) - Full application


