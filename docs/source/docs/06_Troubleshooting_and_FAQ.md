# Troubleshooting and FAQ

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

**Solution Stops (If Broker is Net Running)** 

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
