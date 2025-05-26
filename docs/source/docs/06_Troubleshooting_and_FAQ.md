# Troubleshooting and FAQ

## Installation

**TBD**

## Runtime

### Rendering Issues  

Rendering in GStreamer is generally performed using a sink element. If the specified displaysink element is **not** supported on the current PC environment, the pipeline may exhibit abnormal behavior. Therefore, it is essential to select an appropriate displaysink element based on the system's environment:  

- For CPU-based environments, ximagesink or xvimagesink can be used.  
- For GPU-based environments, glimagesink or similar elements are recommended.  
- Add videoconvert before display sink element  
    ```
    $ gst-launch-1.0 ..... ! videoconvert ! autovideosink
    ```

### Buffer Delays in Sink Element  

When a PC has low performance or is under heavy load, GStreamer pipelines may experience delays in delivering buffers to the sink element (e.g., `ximagesink`, `glimagesink`, etc.). This can result in issues such as  

- Stuttering or lagging video playback.  
- Warning messages like "buffering too slow" or "dropped frames."  
- Overall pipeline performance degradation.  

**Optimize PC Performance**  

- Terminate Background Processes: Free up CPU/GPU resources by closing unnecessary programs.  
- Use Lower-Resolution Videos: Reduce decoding and rendering workload by downscaling input video resolution.  


**Optimize the GStreamer Pipeline**  
Add queue Elements:  

- Insert queue elements at bottleneck points to decouple processing speeds.  

```
gst-launch-1.0 filesrc location=video.mp4 ! decodebin ! queue ! autovideosink
```

Use Asynchronous Rendering:  

- Disable real-time playback synchronization with `sync=false`.  

```
gst-launch-1.0 ... autovideosink sync=false
```

### Kafka Pipeline  

```
%3|1736310124.667|FAIL|rdkafka#producer-1| [thrd:localhost:9092/bootstrap]: localhost:9092/bootstrap: Connect to ipv4#127.0.0.1:9092 failed: Connection refused (after 0ms in state CONNECT)
ERROR: Pipeline doesn't want to pause.
ERROR: from element /GstPipeline:pipeline0/GstDxMsgBroker:dxmsgbroker0: GStreamer error: state change failed and some element failed to post a proper error message with the reason for the failure.
```

The following error might occur when using a message broker with Kafka, as shown in the log above. To resolve this, verify whether the Kafka broker is running. If it is **not** running, perform the steps below to start it and ensure normal operation.  


- Check Running Status  

```
$ ps -ef | grep kafka
```


- If **not** running, follow these steps  

Create a utilities directory and install required dependencies

```
$ mkdir utils && cd utils
$ sudo apt update
$ sudo apt-get install default-jdk
$ wget https://downloads.apache.org/kafka/3.9.0/kafka_2.13-3.9.0.tgz
$ tar -xzf kafka_2.13-3.9.0.tgz
$ cd kafka_2.13-3.9.0
```

In separate terminal sessions, execute the following commands  

**Terminal 1:** Start Zookeeper  

```
$ bin/zookeeper-server-start.sh config/zookeeper.properties
```

**Terminal 2:** Start Kafka Broker  

```
$ bin/kafka-server-start.sh config/server.properties
```

Keep these terminals running while executing the pipeline to ensure proper operation.

---
