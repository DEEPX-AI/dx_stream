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

---

### Display Sink Issues on Raspberry Pi 5

```
(ERROR: from element /GstPipeline:pipeline0/GstFPSDisplaySink:fpsdisplaysink0/GstAutoVideoSink:fps-display-video_sink/GstKMSSink:fps-display-video_sink-actual-sink-kms: GStreamer encountered a general resource error.
Additional debug info:
../sys/kms/gstkmssink.c(2032): gst_kms_sink_show_frame (): /GstPipeline:pipeline0/GstFPSDisplaySink:fpsdisplaysink0/GstAutoVideoSink:fps-display-video_sink/GstKMSSink:fps-display-video_sink-actual-sink-kms:
drmModeSetPlane failed: Permission denied (13)
ERROR: pipeline doesn't want to preroll.
Setting pipeline to NULL ...)
```

The `fpsdisplaysink` element automatically selects an appropriate sink based on the RANK of available rendering sink elements. In this case, the high-ranked `kmssink` was selected, but due to environmental issues, it fails to operate normally.

**Solution**: Modify the pipeline code to replace `fpsdisplaysink` with `ximagesink`, which uses CPU-based rendering in an X11 environment for proper display output.

---

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

---

## Message Broker Advanced Configuration

This section covers advanced configuration for production message broker deployments with security features.

### **MQTT Security Configuration**

#### **SSL/TLS Encryption Setup**

**1. Generate CA Certificate and Server Keys**
```bash
# Create Root CA
openssl genrsa -out ca.key 2048
openssl req -new -x509 -days 360 -key ca.key -out ca.crt -subj "/C=KR/ST=KK/L=SN/O=DXS/OU=Test/CN=TestCA"

# Create Server Certificate (CN must match hostname)
openssl genrsa -out server.key 2048
openssl req -new -out server.csr -key server.key -subj "/C=KR/ST=KK/L=SN/O=DXS/OU=Server/CN=DXS-BROKER"
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -days 360
openssl verify -CAfile ca.crt server.crt
```

**2. Configure Mosquitto Server**
```bash
# Edit /etc/mosquitto/mosquitto.conf
listener 8883
cafile /etc/mosquitto/ca_certificates/ca.crt
certfile /etc/mosquitto/certs/server.crt
keyfile /etc/mosquitto/certs/server.key
```

**3. Setup Authentication**
```bash
# Create user credentials
sudo mosquitto_passwd -c /etc/mosquitto/passwd user
sudo mosquitto_passwd -b /etc/mosquitto/passwd user1 1234
sudo chmod 644 /etc/mosquitto/passwd

# Update mosquitto.conf
echo "password_file /etc/mosquitto/passwd" >> /etc/mosquitto/mosquitto.conf
echo "allow_anonymous false" >> /etc/mosquitto/mosquitto.conf
```

**4. DxMsgBroker Configuration File**
Create `broker_mqtt.cfg`:
```ini
# Authentication
username = user
password = 1234

# Client ID (optional)
client-id = dx_stream_client

# SSL/TLS Configuration
tls_enable = 1
tls_capath = /path/to/ca/certificates
tls_certfile = /path/to/client.crt
tls_keyfile = /path/to/client.key
```

---

### **Kafka Security Configuration**

#### **SSL/TLS Setup for Kafka**

**1. Generate Certificates for Kafka**
Follow the [librdkafka SSL guide](https://github.com/confluentinc/librdkafka/wiki/Using-SSL-with-librdkafka) for detailed certificate generation.

**2. Configure Kafka Server (server.properties)**
```properties
# SSL Configuration
ssl.protocol=TLS
ssl.enabled.protocols=TLSv1.2,TLSv1.1,TLSv1
ssl.keystore.type=JKS
ssl.keystore.location=/path/to/kafka.server.keystore.jks
ssl.keystore.password=server_keystore_password
ssl.key.password=server_key_password
ssl.truststore.type=JKS
ssl.truststore.location=/path/to/kafka.server.truststore.jks
ssl.truststore.password=server_truststore_password
ssl.client.auth=required
```

**3. DxMsgBroker Kafka Configuration**
Create `broker_kafka.cfg`:
```ini
[kafka]
# Message size limits
message.max.bytes=10485760

# SSL Configuration
security.protocol=ssl
ssl.ca.location=/path/to/ca-cert.pem
ssl.certificate.location=/path/to/client-cert.pem
ssl.key.location=/path/to/client-key.pem
ssl.key.password=client_key_password
```

### **Troubleshooting Message Broker Issues**

#### **Common MQTT Problems**

**Connection Refused**
```bash
# Check if Mosquitto is running
sudo systemctl status mosquitto

# Test basic connection
mosquitto_pub -h localhost -p 1883 -t test -m "hello"
```

**SSL Certificate Issues**
```bash
# Verify certificate chain
openssl verify -CAfile ca.crt server.crt

# Test SSL connection
mosquitto_pub -h localhost -p 8883 --cafile ca.crt -t test -m "ssl_test"
```

#### **Common Kafka Problems**

**Consumer Lag**
```bash
# Check consumer group status
bin/kafka-consumer-groups.sh --bootstrap-server localhost:9092 --group dx_stream_group --describe
```

**SSL Handshake Failures**
```bash
# Test SSL connection
bin/kafka-console-producer.sh --bootstrap-server localhost:9093 --topic test \
  --producer.config ssl.properties
```

#### **DX-STREAM Broker Element Issues**

**Library Loading Errors**
- Ensure message converter library path is correct
- Verify library dependencies (json-glib-1.0, etc.)

**Message Format Issues**
- Check if custom library implements all required functions
- Verify JSON output format matches expected structure

**Performance Issues**
- Adjust `message-interval` property to reduce message frequency
- Monitor broker server resources (CPU, memory, network)

---  

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
