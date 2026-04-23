---
name: dx-build-mqtt-kafka-app
description: Build MQTT/Kafka broker application
---

# Skill: Build MQTT/Kafka Broker Application

> **This skill document is sufficient.** Read this FIRST before exploring source code.

## Overview

Build a dx_stream pipeline that publishes inference results to an external message
broker (Kafka or MQTT) using the DxMsgConv and DxMsgBroker elements.

## Output Isolation (MUST FOLLOW)

All AI-generated broker pipeline applications MUST be created under `dx-agentic-dev/`,
NOT in production directories. This prevents accidental modification of existing pipelines.

### Session Directory

```
dx-agentic-dev/<YYYYMMDD-HHMMSS>_<model>_broker/
├── session.json          # Build metadata
├── README.md             # How to run this pipeline
├── run_kafka_producer.sh # Kafka producer pipeline script
├── run_mqtt_publisher.sh # MQTT publisher pipeline script
├── kafka_consumer.py     # Python Kafka consumer (optional)
├── mqtt_subscriber.py    # Python MQTT subscriber (optional)
└── config/               # Broker-specific configs
    ├── broker_kafka.cfg
    ├── broker_mqtt.cfg
    └── msgconv_config.json
```

### session.json Template

```json
{
  "session_id": "<YYYYMMDD-HHMMSS>_<model>_broker",
  "created_at": "<ISO 8601 timestamp>",
  "model": "<model_name>",
  "pipeline_category": "broker",
  "broker_type": "<kafka|mqtt>",
  "status": "complete",
  "notes": "<any relevant notes>"
}
```

### When to Use Production Path

Only create files in production directories when the user EXPLICITLY says:
- "Add this to the production codebase"
- "Create this in the examples directory"
- "Make this a permanent addition"

Default behavior: ALWAYS use `dx-agentic-dev/`.

## Usage

Invoke with `/dx-build-mqtt-kafka-app` or ask the dx-stream-builder agent
to build a broker pipeline. Specify whether you need Kafka or MQTT, and
provide the topic name and broker address.

## Pipeline Pattern

```
source → dxpreprocess → queue → dxinfer → queue → dxpostprocess → queue →
    dxmsgconv → queue → dxmsgbroker
```

Key points:
- DxMsgConv MUST precede DxMsgBroker (converts metadata to JSON)
- DxMsgBroker is a terminal element (no src pad)
- No DxOsd or display sink needed (headless broker pipeline)
- Use `-e` flag with gst-launch-1.0 for clean EOS handling

---

## Kafka Producer Pipeline

### Shell Script

```bash
#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR=$(dirname "$(dirname "$SCRIPT_DIR")")

# Model auto-download
MODEL_NAME="YoloV5S_PPU.dxnn"
MODEL_PATH="$SRC_DIR/samples/models/$MODEL_NAME"
if [ ! -f "$MODEL_PATH" ]; then
    echo "[INFO] $MODEL_NAME not found. Downloading..."
    (cd "$SRC_DIR"/.. && ./setup.sh --model="$MODEL_NAME")
    if [ ! -f "$MODEL_PATH" ]; then
        echo "[ERROR] Failed to download $MODEL_NAME"
        exit 1
    fi
fi

# Kafka broker configuration
KAFKA_BROKER="${KAFKA_BROKER:-localhost:9092}"
KAFKA_TOPIC="${KAFKA_TOPIC:-detections}"

INPUT_VIDEO="${1:-$SRC_DIR/samples/videos/boat.mp4}"

gst-launch-1.0 -e \
    urisourcebin uri=file://$INPUT_VIDEO ! decodebin ! \
    dxpreprocess config-file-path=$SRC_DIR/configs/YoloV5S_PPU/preprocess_config.json ! \
    queue ! \
    dxinfer config-file-path=$SRC_DIR/configs/YoloV5S_PPU/inference_config.json ! \
    queue ! \
    dxpostprocess config-file-path=$SRC_DIR/configs/YoloV5S_PPU/postprocess_config.json ! \
    queue ! \
    dxmsgconv config-file-path=$SRC_DIR/configs/msgconv_config.json ! \
    queue ! \
    dxmsgbroker broker-name=kafka conn-info=$KAFKA_BROKER topic=$KAFKA_TOPIC \
        config=$SRC_DIR/configs/broker_kafka.cfg
```

### Python Kafka Consumer

```python
#!/usr/bin/env python3
"""Kafka consumer for dx_stream inference results."""

import json
import logging
import signal
import sys

from kafka import KafkaConsumer

logger = logging.getLogger(__name__)


def main():
    broker = sys.argv[1] if len(sys.argv) > 1 else 'localhost:9092'
    topic = sys.argv[2] if len(sys.argv) > 2 else 'detections'

    logger.info("Connecting to Kafka broker: %s, topic: %s", broker, topic)

    consumer = KafkaConsumer(
        topic,
        bootstrap_servers=broker,
        value_deserializer=lambda m: json.loads(m.decode('utf-8')),
        auto_offset_reset='latest',
        group_id='dx-stream-consumer'
    )

    def signal_handler(sig, frame):
        logger.info("Shutting down consumer...")
        consumer.close()
        sys.exit(0)

    signal.signal(signal.SIGINT, signal_handler)

    logger.info("Listening for messages...")
    for message in consumer:
        data = message.value
        timestamp = data.get('timestamp', 'N/A')
        objects = data.get('objects', [])

        logger.info("Frame timestamp=%s, detections=%d", timestamp, len(objects))
        for obj in objects:
            logger.info("  class=%s confidence=%.2f bbox=(%.0f,%.0f,%.0f,%.0f)",
                        obj.get('label', 'unknown'),
                        obj.get('confidence', 0),
                        obj.get('x', 0), obj.get('y', 0),
                        obj.get('w', 0), obj.get('h', 0))


if __name__ == '__main__':
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s [%(levelname)s] %(message)s')
    main()
```

---

## MQTT Publisher Pipeline

### Shell Script

```bash
#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR=$(dirname "$(dirname "$SCRIPT_DIR")")

# Model auto-download
MODEL_NAME="YoloV5S_PPU.dxnn"
MODEL_PATH="$SRC_DIR/samples/models/$MODEL_NAME"
if [ ! -f "$MODEL_PATH" ]; then
    echo "[INFO] $MODEL_NAME not found. Downloading..."
    (cd "$SRC_DIR"/.. && ./setup.sh --model="$MODEL_NAME")
    if [ ! -f "$MODEL_PATH" ]; then
        echo "[ERROR] Failed to download $MODEL_NAME"
        exit 1
    fi
fi

# MQTT broker configuration
MQTT_BROKER="${MQTT_BROKER:-localhost:1883}"
MQTT_TOPIC="${MQTT_TOPIC:-detections}"

INPUT_VIDEO="${1:-$SRC_DIR/samples/videos/boat.mp4}"

gst-launch-1.0 -e \
    urisourcebin uri=file://$INPUT_VIDEO ! decodebin ! \
    dxpreprocess config-file-path=$SRC_DIR/configs/YoloV5S_PPU/preprocess_config.json ! \
    queue ! \
    dxinfer config-file-path=$SRC_DIR/configs/YoloV5S_PPU/inference_config.json ! \
    queue ! \
    dxpostprocess config-file-path=$SRC_DIR/configs/YoloV5S_PPU/postprocess_config.json ! \
    queue ! \
    dxmsgconv config-file-path=$SRC_DIR/configs/msgconv_config.json ! \
    queue ! \
    dxmsgbroker broker-name=mqtt conn-info=$MQTT_BROKER topic=$MQTT_TOPIC
```

### Python MQTT Subscriber

```python
#!/usr/bin/env python3
"""MQTT subscriber for dx_stream inference results."""

import json
import logging
import sys

import paho.mqtt.client as mqtt

logger = logging.getLogger(__name__)


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        logger.info("Connected to MQTT broker")
        client.subscribe(userdata['topic'])
        logger.info("Subscribed to topic: %s", userdata['topic'])
    else:
        logger.error("Connection failed with code: %d", rc)


def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode('utf-8'))
        timestamp = data.get('timestamp', 'N/A')
        objects = data.get('objects', [])

        logger.info("Frame timestamp=%s, detections=%d", timestamp, len(objects))
        for obj in objects:
            logger.info("  class=%s confidence=%.2f bbox=(%.0f,%.0f,%.0f,%.0f)",
                        obj.get('label', 'unknown'),
                        obj.get('confidence', 0),
                        obj.get('x', 0), obj.get('y', 0),
                        obj.get('w', 0), obj.get('h', 0))
    except json.JSONDecodeError as e:
        logger.error("Failed to parse message: %s", e)


def main():
    broker_host = sys.argv[1] if len(sys.argv) > 1 else 'localhost'
    broker_port = int(sys.argv[2]) if len(sys.argv) > 2 else 1883
    topic = sys.argv[3] if len(sys.argv) > 3 else 'detections'

    client = mqtt.Client()
    client.user_data_set({'topic': topic})
    client.on_connect = on_connect
    client.on_message = on_message

    logger.info("Connecting to %s:%d", broker_host, broker_port)
    client.connect(broker_host, broker_port, keepalive=60)

    try:
        client.loop_forever()
    except KeyboardInterrupt:
        logger.info("Shutting down...")
        client.disconnect()


if __name__ == '__main__':
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s [%(levelname)s] %(message)s')
    main()
```

---

## JSON Message Format

DxMsgConv produces JSON messages with this structure:

```json
{
    "timestamp": 1234567890123,
    "frame_id": 42,
    "source": "file:///path/to/video.mp4",
    "objects": [
        {
            "class_id": 0,
            "label": "person",
            "confidence": 0.92,
            "x": 100.0,
            "y": 150.0,
            "w": 80.0,
            "h": 200.0,
            "track_id": -1
        },
        {
            "class_id": 2,
            "label": "car",
            "confidence": 0.87,
            "x": 300.0,
            "y": 250.0,
            "w": 150.0,
            "h": 100.0,
            "track_id": -1
        }
    ]
}
```

Fields are configurable via `msgconv_config.json`.

---

## Configuration Files

### broker_kafka.cfg

```ini
[kafka]
bootstrap.servers=localhost:9092
compression.type=none
queue.buffering.max.messages=100000
batch.num.messages=1
```

### broker_mqtt.cfg

```ini
[mqtt]
client.id=dx-stream-publisher
clean.session=true
qos=1
```

### msgconv_config.json

```json
{
    "msg_format": "json",
    "include_timestamp": true,
    "include_frame_id": true,
    "include_source": true,
    "include_objects": true
}
```

---

## Prerequisites

### Kafka

```bash
# Install Kafka (or use Docker)
docker run -d --name kafka \
    -p 9092:9092 \
    -e KAFKA_ADVERTISED_LISTENERS=PLAINTEXT://localhost:9092 \
    confluentinc/cp-kafka:latest

# Python consumer dependency
pip install kafka-python
```

### MQTT

```bash
# Install Mosquitto broker
sudo apt install mosquitto mosquitto-clients

# Start broker
sudo systemctl start mosquitto

# Python subscriber dependency
pip install paho-mqtt
```
