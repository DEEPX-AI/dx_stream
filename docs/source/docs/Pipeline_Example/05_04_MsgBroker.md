The pipeline demonstrates how to process a local video file with the YOLOv7 model for object detection, convert the results into a structured message format (e.g., JSON), and publish them to a message broker such as MQTT or Kafka for downstream applications.

![](./../../resources/05_04_msgbroker.png)

The pipeline in the figure is defined in 
`dx_stream/pipelines/broker/*.sh` and can be used as a reference for execution.

### **Explanation**

**Element Descriptions**  

- **`dxmsgconv`**: Transform element that processes inference metadata from upstream **DxPostprocess** elements and converts it into structured message payloads (typically JSON) using a user-defined custom library. The element requires a `library-file-path` property pointing to the custom message conversion library.
- **`dxmsgbroker`**: Sink element that transmits payload messages to external message brokers (MQTT or Kafka) using a **Broker Abstraction Layer (BAL)** that provides unified interface for different broker types. 

### **Usage Notes**  

**Pipeline Execution**  
1. **Server Side**: Run the DX-STREAM pipeline with broker output on the processing server
2. **Client Side**: Run the consumer application to receive and process messages

**Basic Properties**  
- **`dxmsgconv`**: Converts inference metadata to JSON format
- **`dxmsgbroker`**: Publishes messages to MQTT/Kafka broker

For detailed element properties, see the Elements documentation.

---

### **Quick Start Demo**

This section demonstrates how to run a basic message broker pipeline demo using MQTT or Kafka.

#### **MQTT Demo**

**1. Server Setup (Processing Server with Message Broker)**
```bash
# Install and start Mosquitto MQTT broker
sudo apt install mosquitto mosquitto-clients
sudo systemctl start mosquitto
sudo systemctl enable mosquitto
```

**2. Run DX-STREAM Pipeline (Server Side)**
```bash
# Execute the broker pipeline on processing server
# The pipeline uses YOLOv7 model for object detection and converts to JSON messages
cd /path/to/dx_stream
./pipelines/broker/run_dxmsgbroker_mqtt.sh

# Or run manually with custom settings
gst-launch-1.0 \
  urisourcebin uri=file:///path/to/your/video.mp4 ! decodebin ! \
  dxpreprocess config-file-path=configs/Object_Detection/YoloV7/preprocess_config.json ! \
  dxinfer config-file-path=configs/Object_Detection/YoloV7/inference_config.json ! \
  dxpostprocess config-file-path=configs/Object_Detection/YoloV7/postprocess_config.json ! \
  dxmsgconv library-file-path=/usr/lib/libdx_msgconvl.so config-file-path=configs/msgconv_config.json ! \
  dxmsgbroker broker-name=mqtt conn-info=localhost:1883 topic=test
```

**3. Client Application (Any Machine - Consumer Only)**
```bash
# Install Python MQTT client library (client machine only needs this)
pip install paho-mqtt

# Run the MQTT client to receive messages from server
python3 /usr/share/dx-stream/bin/mqtt_sub_example.py -n <server_ip> -p 1883 -t test

# Or for C++ version
mqtt_sub_example -h <server_ip> -t test -p 1883
```

#### **Kafka Demo**

**1. Server Setup (Processing Server with Message Broker)**
```bash
# Install Java and Kafka
sudo apt install default-jdk
wget https://downloads.apache.org/kafka/3.9.0/kafka_2.13-3.9.0.tgz
tar -xzf kafka_2.13-3.9.0.tgz
cd kafka_2.13-3.9.0

# Start Zookeeper and Kafka server
bin/zookeeper-server-start.sh config/zookeeper.properties &
bin/kafka-server-start.sh config/server.properties &
```

**2. Run DX-STREAM Pipeline (Server Side)**
```bash
# Execute the broker pipeline on processing server  
cd /path/to/dx_stream
./pipelines/broker/run_dxmsgbroker_kafka.sh

# Or run manually with custom settings
gst-launch-1.0 \
  urisourcebin uri=file:///path/to/your/video.mp4 ! decodebin ! \
  dxpreprocess config-file-path=configs/Object_Detection/YoloV7/preprocess_config.json ! \
  dxinfer config-file-path=configs/Object_Detection/YoloV7/inference_config.json ! \
  dxpostprocess config-file-path=configs/Object_Detection/YoloV7/postprocess_config.json ! \
  dxmsgconv library-file-path=/usr/lib/libdx_msgconvl.so config-file-path=configs/msgconv_config.json ! \
  dxmsgbroker broker-name=kafka conn-info=localhost:9092 topic=test
```

**3. Client Application (Any Machine - Consumer Only)**
```bash
# Install Python Kafka client library (client machine only needs this)
pip install kafka-python

# Run the Kafka client to receive messages from server
python3 /usr/share/dx-stream/bin/kafka_consume_example.py -n <server_ip> -p 9092 -t test

# Or for C++ version
kafka_consume_example -n <server_ip> -p 9092 -t test
```

#### **Network Architecture**

```
┌─────────────────────────┐    ┌─────────────────────────┐
│   Processing Server     │    │    Client Machine       │
│                         │    │                         │
│ ┌─────────────────────┐ │    │ ┌─────────────────────┐ │
│ │   DX-STREAM         │ │    │ │  Consumer App       │ │
│ │   Pipeline          │ │    │ │  (Python/C++)       │ │
│ │                     │ │────┤ │                     │ │
│ │ dxmsgconv           │ │    │ │  - Only needs       │ │
│ │ dxmsgbroker         │ │    │ │    client library   │ │
│ └─────────────────────┘ │    │ │  - No broker        │ │
│                         │    │ │    service needed   │ │
│ ┌─────────────────────┐ │    │ └─────────────────────┘ │
│ │ Message Broker      │ │    │                         │
│ │ (MQTT/Kafka)        │ │    │                         │
│ └─────────────────────┘ │    │                         │
└─────────────────────────┘    └─────────────────────────┘
```

**Requirements:**
- **Server**: DX-STREAM + Message Broker Service (MQTT/Kafka)
- **Client**: Only consumer application + client library (paho-mqtt/kafka-python)

**Note:** Replace `<server_ip>` with the actual IP address of your processing server. For local testing, use `localhost`.

#### **Pipeline Properties**

The broker pipelines use the following key properties:

**DxMsgConv Element:**
- `config-file-path`: Path to configuration file containing message format properties (optional)
- `library-file-path`: Path to custom message converter library (**required**)
- `message-interval`: Frame interval for message conversion (default: 1)

**DxMsgBroker Element:**
- `broker-name`: Message broker type - "mqtt" or "kafka" (**required**)
- `conn-info`: Connection string in format `host:port` (**required**)
- `topic`: Topic name for message publishing (**required**)
- `config`: Path to broker configuration file for advanced settings (optional)

---

### **Advanced Configuration**

For production deployments with SSL/TLS security, authentication, and advanced broker configurations, refer to **Chapter 6. Troubleshooting and FAQ**.---
