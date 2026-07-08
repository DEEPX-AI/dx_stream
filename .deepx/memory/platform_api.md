# Platform API — dx_stream

DX-RT platform API relevant to dx_stream pipeline operations.

---

## NPU Discovery

```bash
# Check if NPU is detected and ready
dxrt-cli -s

# Expected output includes:
# Device: DX-M1
# Status: Ready
# Temperature: 45C
# Utilization: 0%
```

If `dxrt-cli` is not found, DX-RT is not installed. Install it first.

---

## GStreamer Plugin Registration

```bash
# Verify dx_stream plugin is registered
gst-inspect-1.0 dxinfer

# List all dx_stream elements
gst-inspect-1.0 | grep dxstream

# Expected: 13 elements listed under 'dxstream' plugin
# dxstream: dxpreprocess
# dxstream: dxinfer
# dxstream: dxpostprocess
# dxstream: dxtracker
# dxstream: dxosd
# dxstream: dxgather
# dxstream: dxinputselector
# dxstream: dxoutputselector
# dxstream: dxrate
# dxstream: dxmsgconv
# dxstream: dxmsgbroker
# dxstream: dxscale
# dxstream: dxconvert
```

If elements are not listed:
1. Run `./install.sh && ./build.sh` from dx_stream root
2. Check `GST_PLUGIN_PATH` includes the dx_stream plugin directory
3. Verify DX-RT version compatibility

---

## Version Compatibility

```bash
# Check DX-RT version
dpkg -l | grep dx-runtime
# or
dxrt-cli --version

# Check dx_stream version
cat release.ver  # from dx_stream root

# Check GStreamer version
gst-launch-1.0 --version
```

**Minimum requirements:**
- DX-RT >= 3.0.0
- GStreamer >= 1.14.0
- Python >= 3.8 (for pydxs bindings)

---

## Driver Status

```bash
# Check kernel module is loaded
lsmod | grep dxnpu

# Check device node exists
ls -la /dev/dxnpu*

# Check driver messages
dmesg | grep -i dxnpu
```

---

## Environment Variables

| Variable | Purpose | Example |
|---|---|---|
| `GST_PLUGIN_PATH` | Additional GStreamer plugin search paths | `/usr/local/lib/gstreamer-1.0` |
| `GST_DEBUG` | GStreamer debug level | `3` or `dxinfer:5` |
| `GST_DEBUG_FILE` | Log debug output to file | `/tmp/gst-debug.log` |
| `GST_TRACERS` | Enable GStreamer tracers | `framerate;latency` |
| `DISPLAY` | X11 display for video sinks | `:0` |

---

## Platform Verification Script

```bash
#!/bin/bash
echo "=== DX-RT Platform Check ==="

# 1. NPU
echo -n "NPU: "
if dxrt-cli -s > /dev/null 2>&1; then
    echo "DETECTED"
else
    echo "NOT FOUND (install DX-RT)"
fi

# 2. GStreamer
echo -n "GStreamer: "
if gst-launch-1.0 --version > /dev/null 2>&1; then
    gst-launch-1.0 --version 2>&1 | head -1
else
    echo "NOT FOUND"
fi

# 3. dx_stream plugin
echo -n "dx_stream plugin: "
if gst-inspect-1.0 dxinfer > /dev/null 2>&1; then
    ELEMENTS=$(gst-inspect-1.0 2>/dev/null | grep -c dxstream)
    echo "REGISTERED ($ELEMENTS elements)"
else
    echo "NOT REGISTERED (run install.sh + build.sh)"
fi

# 4. Display
echo -n "Display: "
if [ -n "$DISPLAY" ]; then
    echo "AVAILABLE ($DISPLAY)"
else
    echo "HEADLESS (will use fakesink)"
fi

# 5. Postprocess libraries
echo -n "Postprocess libs: "
LIBS=$(ls /usr/local/share/gstdxstream/lib/libpostprocess_*.so 2>/dev/null | wc -l)
echo "$LIBS libraries installed"

echo "=== Check Complete ==="
```
