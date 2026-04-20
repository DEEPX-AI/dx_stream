# Troubleshooting and FAQ

## Debug Logging

#### **Overview**

DX-STREAM provides comprehensive debug logging through GStreamer's `GST_DEBUG` system. This allows you to monitor element behavior, trace buffer flow, analyze performance, and diagnose issues at various levels of detail.

#### **Basic Usage**

**Enable debug logging for all DX-STREAM elements:**

```bash
$ GST_DEBUG=dx*:4 ./your_application
```

**Enable debug logging for specific elements:**

```bash
$ GST_DEBUG=dxinfer:4,dxtracker:4 ./your_application
```

#### **Debug Levels**

| **Level** | **Name**    | **When to Use**                                                   |
|-----------|-------------|-------------------------------------------------------------------|
| 1         | ERROR       | Critical failures preventing operation                            |
| 2         | WARNING     | Non-fatal issues that may affect behavior                         |
| 3         | INFO        | State changes and configuration confirmation                      |
| 4         | DEBUG       | Detailed operational information for troubleshooting              |

#### **Common Scenarios**

**Monitor inference performance:**

```bash
$ GST_DEBUG=dxinfer:4 ./your_app 2>&1 | grep "completed in"
```

**Debug configuration loading:**

```bash
$ GST_DEBUG=dx*:3 ./your_app 2>&1 | grep "Loaded"
```

**Track buffer flow:**

```bash
$ GST_DEBUG=dx*:4 ./your_app 2>&1 | grep "Processing buffer"
```

**Identify dropped frames:**

```bash
$ GST_DEBUG=dxpreprocess:4,dxinfer:4 ./your_app 2>&1 | grep "Dropping"
```

**Save logs to file:**

```bash
$ GST_DEBUG=dx*:4 GST_DEBUG_FILE=/tmp/debug.log ./your_app
```

!!! tip "Detailed Guide"

    For comprehensive debugging strategies and element-specific logging information, see **Chapter 07 - Debugging Guide**.

---

## Rendering Issues  

#### **Problem: Abnormal Behavior**

The pipeline may exhibit abnormal behavior or fail to display video when attempting to render the video stream on the screen.

#### **Cause: Unsupported Element**

The root cause of abnormal behavior is:
-	Incompatibility: The specified displaysink element is not supported by the current PC environment (e.g., due to unsupported graphics drivers or display server settings).

#### **Solution: Element Selection & Compatibility**

It's essential to address both the hardware fit and the data format compatibility.

**Solution 1: Element Selection**

- For CPU-based Environments: Use ximagesink or xvimagesink.

- For GPU-based Environments: Use glimagesink or similar GPU-accelerated elements.


**Solution 2: Ensure Compatibility**

Mandatory Step: Always add videoconvert directly before the displaysink element.

- Purpose: This element correctly converts the video format from upstream elements into a format the chosen sink can process, preventing format mismatch errors.

    ```
    $ gst-launch-1.0 .... ! videoconvert ! autovideosink
    ```

---

## Display Sink Issues on Raspberry Pi 5

```
(ERROR: from element /GstPipeline:pipeline0/GstFPSDisplaySink:fpsdisplaysink0/GstAutoVideoSink:fps-display-video_sink/GstKMSSink:fps-display-video_sink-actual-sink-kms: GStreamer encountered a general resource error.
Additional debug info:
../sys/kms/gstkmssink.c(2032): gst_kms_sink_show_frame (): /GstPipeline:pipeline0/GstFPSDisplaySink:fpsdisplaysink0/GstAutoVideoSink:fps-display-video_sink/GstKMSSink:fps-display-video_sink-actual-sink-kms:
drmModeSetPlane failed: Permission denied (13)
ERROR: pipeline doesn't want to preroll.
Setting pipeline to NULL ...)
```

#### **Problem: Rendering Failure**

The GStreamer pipeline fails to preroll and terminates prematurely, resulting in a general resource error. The specific error message indicates a system access issue: drmModeSetPlane failed: Permission denied (13).

#### **Cause: High-Ranked KMSSink Failure**

The issue stems from the automatic selection process of the sink element:

**Automatic Selection**: The fpsdisplaysink element automatically selects the rendering sink with the highest RANK.

**The Culprit**: In this environment, the high-ranked kmssink was automatically chosen.

**Environmental Failure**: Due to environmental issues, likely related to permissions or system configuration specific to the Raspberry Pi 5 setup, the selected kmssink failed to operate normally, leading to the "Permission denied" error.

#### **Solution: Manual Sink Replacement**

The solution involves overriding the automatic, failing selection by manually specifying a stable, compatible sink element.

**Action**: Modify the pipeline code to **replace the fpsdisplaysink element with ximagesink**.

**Result**: **ximagesink uses CPU-based rendering** within an X11 environment, circumventing the resource and permission issues associated with kmssink to ensure proper display output.


---

## Display Sink Issues on Orange Pi 5 Plus with Debian 12

#### **Problem: Corrupted or Distorted Video Rendering**

When running DX-STREAM pipeline scripts on **Orange Pi 5 Plus with Debian 12**, you may experience corrupted, distorted, or improperly rendered video output. The video display may appear garbled, with incorrect colors, flickering, or visual artifacts that make the output unusable.

**Affected Environment**:  

- **Hardware**: Orange Pi 5 Plus (RK3588 chipset)  
- **Operating System**: Debian 12 (official image from [Orange Pi download page](http://www.orangepi.org/html/hardWare/computerAndMicrocontrollers/service-and-support/Orange-Pi-5-plus.html))  
- **Affected Component**: Video rendering pipeline (specifically the fpsdisplaysink element)  

#### **Cause: GStreamer Format Negotiation Bug**

This is a **GStreamer bug specific to the Orange Pi 5 Plus + Debian 12 environment**. The issue occurs during format negotiation between the `videoconvert` element and the `fpsdisplaysink` element:

(1) **Format Mismatch**: The default video format negotiated by `videoconvert` is incompatible with the sink element selected by `fpsdisplaysink` in this environment.  

(2) **Environment-Specific**: This bug was observed with the official Debian 12 image for Orange Pi 5 Plus. It may or may not occur depending on:  

   - The specific Debian 12 ISO image version used  
   - System library versions (GStreamer, graphics drivers, etc.)  
   - Display server configuration (X11/Wayland)  

(3) **Not Universal**: While primarily seen on Orange Pi 5 Plus + Debian 12, **similar rendering issues could potentially occur on other ARM-based SBCs or platforms** with comparable GStreamer + display stack configurations.  

#### **Solution: Force I420 Format Conversion**

The workaround is to explicitly force the video format to **I420** before passing it to `fpsdisplaysink`, which bypasses the faulty format negotiation.

**Action**: In the affected pipeline script, uncomment the Orange Pi 5 Plus workaround code block.

**Before (default - may cause rendering issues)**:
```bash
# Default videoconvert pipeline
VIDEOCONVERT_PIPELINE="videoconvert"

# NOTE: If you experience rendering issues (corrupted/distorted video output) on Orange Pi 5 Plus
# with Debian 12, uncomment the lines below to force I420 format conversion.
# See troubleshooting documentation for more details.
# if grep -q "rk3588" /proc/device-tree/compatible 2>/dev/null; then
#     if [ "$(lsb_release -rs)" = "12" ]; then
#         echo "Detected Orange Pi 5 Plus with Debian 12 - using I420 format"
#         VIDEOCONVERT_PIPELINE="videoconvert ! video/x-raw,format=I420"
#     fi
# fi
```

**After (uncommented - fixes rendering issues)**:
```bash
# Default videoconvert pipeline
VIDEOCONVERT_PIPELINE="videoconvert"

# NOTE: If you experience rendering issues (corrupted/distorted video output) on Orange Pi 5 Plus
# with Debian 12, uncomment the lines below to force I420 format conversion.
# See troubleshooting documentation for more details.
if grep -q "rk3588" /proc/device-tree/compatible 2>/dev/null; then
    if [ "$(lsb_release -rs)" = "12" ]; then
        echo "Detected Orange Pi 5 Plus with Debian 12 - using I420 format"
        VIDEOCONVERT_PIPELINE="videoconvert ! video/x-raw,format=I420"
    fi
fi
```

**Location**: This code block appears in all DX-STREAM pipeline scripts that use video display, including:  

- `dx_stream/pipelines/single_network/*/run_*.sh`  
- `dx_stream/pipelines/multi_stream/run_*.sh`  
- `dx_stream/pipelines/rtsp/run_*.sh`  
- `dx_stream/pipelines/tracking/run_*.sh`  
- `dx_stream/pipelines/secondary_mode/run_*.sh`  

**Result**: Forcing the `I420` format ensures proper format negotiation and eliminates the rendering corruption, allowing normal video display.

!!! note "When to Apply This Workaround"
    - **Always apply** if you're using Orange Pi 5 Plus with official Debian 12 image and experiencing rendering issues
    - **Try first without workaround** on other Debian versions or custom images - it may work fine
    - **Consider applying** if you experience similar rendering issues on other ARM SBCs with similar software stacks
    - **No harm in enabling** - forcing I420 format is a safe operation that only affects format negotiation


---

## Buffer Delays in Sink Element  

#### **Problem & Symptoms**

The core problem is a performance bottleneck in the system, leading to noticeable playback degradation and warning messages.

- Stuttering or lagging video playback.
- Pipeline performance degradation.
- Warning messages in the console, such as "buffering too slow" or "dropped frames."

#### **Solutions: Performance Optimization**

Solutions focus on optimizing both the PC environment and the GStreamer pipeline structure.

**Solution 1: Optimize PC Performance**

- **Terminate Background Processes**: Free up CPU/GPU resources by closing any unnecessary programs running in the background.

- **Use Lower-Resolution Videos**: Reduce the decoding and rendering workload by using lower input video resolutions or downscaling the video stream early in the pipeline.

**Solution 2: Optimize the GStreamer Pipeline**

- **1. Add queue Elements (Decoupling):**

    Insert queue elements at potential bottleneck points to decouple (separate) processing speeds between adjacent elements. This allows faster elements to process data ahead, mitigating delays caused by slower elements.

    Example:

    ```
    gst-launch-1.0 filesrc location=video.mp4 ! decodebin ! queue ! autovideosink
    ```

- **2.	Use Asynchronous Rendering (Disable Synchronization):**

    Disable real-time playback synchronization by setting sync=false on the sink element. This tells the sink to render frames as quickly as possible, ignoring the clock and often reducing perceived lag, though it may result in playback that is faster or slower than real-time.

    Example:

    ```
    gst-launch-1.0 ... autovideosink sync=false
    ```

---

## Troubleshooting Message Broker Issues

#### **A. Common MQTT Problems**

**Connection Refused**

- Check Broker Status

  ```bash
  # Verify if Mosquitto is running
  sudo systemctl status mosquitto
  ```

- Test Basic Connection

  ```bash
  # Attempt a basic publish operation
  mosquitto_pub -h localhost -p 1883 -t test -m "hello"
  ```

**SSL Certificate Issues**

- Verify Chain

  ```bash
  # Verify the certificate chain integrity
  openssl verify -CAfile ca.crt server.crt
  ```

- Test SSL Connection

  ```bash
  # Test secure publishing
  mosquitto_pub -h localhost -p 8883 --cafile ca.crt -t test -m "ssl_test"
  ```

#### **B. Common Kafka Problems**

**Consumer Lag**

- Check Consumer Group Status
  ```bash
  # monitor the consumer group status to identify lag
  bin/kafka-consumer-groups.sh --bootstrap-server localhost:9092 --group dx_stream_group --describe
  ```

**SSL Handshake Failures**
- Test SSL Connection
  ```bash
  # Test SSL connection
  bin/kafka-console-producer.sh --bootstrap-server localhost:9093 --topic test \
    --producer.config ssl.properties
  ```

#### **C. DX-STREAM Broker Element Issues**

**Library Loading Errors**

- Ensure message converter library path is correct
- Verify library dependencies (json-glib-1.0, etc.) are installed and linked.

**Message Format Issues**

- Check if custom library implements all required functions
- Verify that the final JSON output format matches expected structure

**Performance Issues**

- Adjust `message-interval` property to reduce message frequency
- Monitor the broker server resources (CPU, memory, network)


#### **D. Critical Kafka Connection Refusal Error** 

A common error when using Kafka is connection refusal, as indicated by the following log.

```
%3|1736310124.667|FAIL|rdkafka#producer-1| [thrd:localhost:9092/bootstrap]: localhost:9092/bootstrap: Connect to ipv4#127.0.0.1:9092 failed: Connection refused (after 0ms in state CONNECT)
ERROR: Pipeline doesn't want to pause.
ERROR: from element /GstPipeline:pipeline0/GstDxMsgBroker:dxmsgbroker0: GStreamer error: state change failed and some element failed to post a proper error message with the reason for the failure.
```

This error usually means the Kafka broker is not running. To resolve this, verify its status and restart it if necessary.

**Solution Steps (If Broker is Not Running)** 

- Check Running Status  

    ```
    $ ps -ef | grep kafka
    ```


- Installation and Startup (If not running)

    Installation: Create a utilities directory and install required dependencies (Java JDK is essential). Download and extract the Kafka distribution:

    ```
    $ mkdir utils && cd utils
    $ sudo apt update
    $ sudo apt-get install default-jdk
    $ wget https://downloads.apache.org/kafka/3.9.0/kafka_2.13-3.9.0.tgz
    $ tar -xzf kafka_2.13-3.9.0.tgz
    $ cd kafka_2.13-3.9.0
    ```

    Start Zookeeper (terminal 1): Kafka requires Zookeeper to be running first

    ```
    $ bin/zookeeper-server-start.sh config/zookeeper.properties
    ```

    Start Kafka Broker (Terminal 2): In a separate terminal session, start the Kafka broker

    ```
    $ bin/kafka-server-start.sh config/server.properties
    ```

!!! note "NOTE" 
  
    Keep both terminal sessions running while the DX-STREAM pipeline is active to ensure proper operation.

---

## Build Issues with Meson Installation

#### **Overview: How `build.sh` Handles `meson install`**

DX-STREAM uses `sudo meson install` to install plugins into system directories (e.g., `/usr/local/`).  
Since `sudo` runs as root and does not inherit the user's Python environment, `build.sh` explicitly passes the Python package paths via `PYTHONPATH`:

```bash
sudo env PYTHONPATH="$(python3 -c '...')" "$(which meson)" install -C "${BUILD_DIR}" --no-rebuild
```

This command resolves two things from the **current shell** before `sudo` execution:

- `$(which meson)` — locates the meson binary
- `$(python3 -c '...')` — collects all Python site-packages paths

---

#### **Supported Meson Installation Methods**

The following single-meson-installation environments are fully supported:

| Installation Method | Binary Location | Module Location | Supported |
|---|---|---|:---:|
| `pip install --user meson` | `~/.local/bin/meson` | `~/.local/lib/python3.x/site-packages/` | ✅ |
| `pip install meson` (in venv) | `venv/bin/meson` | `venv/lib/python3.x/site-packages/` | ✅ |
| `sudo pip install meson` | `/usr/local/bin/meson` | `/usr/local/lib/python3.x/dist-packages/` | ✅ |
| `apt install meson` | `/usr/bin/meson` | `/usr/lib/python3/dist-packages/` | ✅ |

---

#### **Known Limitation: Multiple Meson Installations**

!!! warning "WARNING"

    If meson is installed in **more than one location** simultaneously, a version mismatch may occur between the meson binary and its Python module.

When multiple meson installations coexist, the `meson` binary resolved by `which meson` and the `mesonbuild` Python module loaded at runtime may come from **different installations**:

```bash
# Example: meson binary from venv (v1.4), but module from ~/.local/ (v0.61)
$ which meson
/home/user/venv/bin/meson            # meson 1.4

$ python3 -c "import mesonbuild; print(mesonbuild.__file__)"
/home/user/.local/.../mesonbuild/    # meson 0.61 (loaded first by import order)
```

This is a **fundamental limitation** of the `sudo + PYTHONPATH` pattern and cannot be fully resolved by any PYTHONPATH construction strategy.

**Symptoms:**

- `meson install` fails with unexpected errors or tracebacks
- Build succeeds but installed files are incorrect or incomplete
- `ModuleNotFoundError: No module named 'mesonbuild'`

---

#### **Solution: Keep a Single Meson Installation**

**Step 1.** Check for multiple installations:

```bash
# List all meson binaries in PATH
which -a meson

# Check pip-installed meson
pip show meson 2>/dev/null && echo "Found: pip (user/venv)"
pip3 show meson 2>/dev/null && echo "Found: pip3"

# Check system package
apt list --installed 2>/dev/null | grep meson

# Verify binary and module point to the same installation
which meson
python3 -c "import mesonbuild, os; print(os.path.dirname(os.path.dirname(mesonbuild.__file__)))"
```

**Step 2.** Remove duplicates, keeping only the intended one:

```bash
# Remove user-level installation
pip uninstall meson

# Or remove system-level installation
sudo apt remove meson

# Or remove from venv
pip uninstall meson  # with venv activated
```

**Step 3.** Verify single installation:

```bash
$ which meson
/home/user/.local/bin/meson

$ python3 -c "import mesonbuild, os; print(os.path.dirname(os.path.dirname(mesonbuild.__file__)))"
/home/user/.local/lib/python3.8/site-packages

# Both should share the same installation prefix (~/.local/ in this example)
```
