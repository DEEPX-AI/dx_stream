# Kafka Consumer Example

This example receives and displays JSON messages published by `dxmsgbroker` in a DX-Stream pipeline via Kafka.
When `dxmsgconv` is configured with `include-frame=true`, base64-encoded JPEG frame data in the JSON can be decoded and displayed in a window.

## Prerequisites

### Broker Service (Server Side)
Zookeeper and Kafka server must be running.

```bash
# Install
sudo apt install default-jdk
wget https://downloads.apache.org/kafka/3.9.2/kafka_2.13-3.9.2.tgz
tar -xzf kafka_2.13-3.9.2.tgz
cd kafka_2.13-3.9.2

# Start Zookeeper
bin/zookeeper-server-start.sh config/zookeeper.properties &

# Start Kafka server
bin/kafka-server-start.sh config/server.properties &
```
Default port: **9092**

> **Remote access:** If the consumer runs on a different machine from the broker,
> you must set `advertised.listeners` in `config/server.properties` to the broker's IP:
> ```properties
> advertised.listeners=PLAINTEXT://<broker_ip>:9092
> ```
> Then restart the Kafka server. Without this, remote consumers will fail to resolve the broker's internal hostname.

### Producer Side (DX-Stream Pipeline)
The DX-Stream pipeline publishes messages via `dxmsgconv` + `dxmsgbroker`.

```bash
# Using the provided script
./pipelines/broker/run_dxmsgbroker_kafka.sh

# Or run manually
gst-launch-1.0 \
  urisourcebin uri=file:///path/to/video.mp4 ! decodebin ! \
  dxpreprocess config-file-path=configs/.../preprocess_config.json ! \
  dxinfer config-file-path=configs/.../inference_config.json ! \
  dxpostprocess config-file-path=configs/.../postprocess_config.json ! \
  dxmsgconv library-file-path=/usr/lib/libdx_msgconvl.so include-frame=true ! \
  dxmsgbroker broker-name=kafka conn-info=localhost:9092 topic=test
```

> Setting `include-frame=true` includes base64-encoded JPEG frame data in the JSON payload.

### Consumer Side (This Example App)
Only the **client libraries** are required — no broker service needed on the consumer machine.

#### C++ Build Dependencies
```bash
sudo apt install librdkafka-dev libjson-glib-dev libopencv-dev
```

#### Python Dependencies
```bash
pip install confluent-kafka opencv-python
```

## Build (C++)

```bash
cd dx_stream/apps/kafka
meson setup builddir
meson compile -C builddir
```

Or from the parent directory:
```bash
cd dx_stream/apps && ./build.sh
```

## Usage

### C++
```bash
# Summary output (default)
./kafka_consume_example -n <broker_host> -t <topic>

# Print full JSON
./kafka_consume_example -n <broker_host> -t <topic> -a

# Display frames in a window
./kafka_consume_example -n <broker_host> -t <topic> -d
```

### Python
```bash
# Summary output (default)
python kafka_consume_example.py -n <broker_host> -t <topic>

# Print full JSON
python kafka_consume_example.py -n <broker_host> -t <topic> -a

# Display frames in a window
python kafka_consume_example.py -n <broker_host> -t <topic> -d
```

## Options

| Option | C++ | Python | Description |
|--------|-----|--------|-------------|
| Broker host | `-n <host>` | `-n <host>` | Kafka broker address |
| Topic | `-t <topic>` | `-t <topic>` | Kafka topic to subscribe to |
| Port | `-p <port>` | `-p <port>` | Broker port (default: 9092) |
| Print all | `-a` | `-a` | Print full JSON (frameData replaced with `<base64 omitted>`) |
| Display | `-d` | `-d` | Decode and display JPEG frames via cv::imshow |

## Output Example

```
# Default output
Received payload 1234 bytes | seqId: 1 | objects: 3 | frameData: no

# With include-frame enabled
Received payload 85432 bytes | seqId: 1 | objects: 3 | frameData: yes
```

> Use `localhost` as `<broker_host>` for local testing.
> For SSL/TLS configuration, refer to the [MsgBroker Pipeline documentation](../../docs/source/docs/Pipeline_Example/05_04_MsgBroker.md).
