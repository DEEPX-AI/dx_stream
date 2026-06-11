# Prompt: New MQTT/Kafka Broker Application

Create a dx_stream pipeline that publishes inference results to **{broker_type}**.

## Parameters

- **Broker Type**: `{broker_type}` (kafka | mqtt)
- **Broker Host**: `{broker_host}` (e.g., localhost:9092 for Kafka, localhost:1883 for MQTT)
- **Topic**: `{topic}` (e.g., detections)
- **Model**: `{model_name}` (from model_list.json)
- **Input**: `{input_source}` (file path | usb | rtsp://...)

## Steps

### 1. Compose Broker Pipeline

Read `.deepx/skills/dx-agent-stream-build-mqtt-kafka.md` for the broker pattern.

Pipeline: `source → dxpreprocess → dxinfer → dxpostprocess → dxmsgconv → dxmsgbroker`

Key properties:
- DxMsgConv: `config-file-path` for message format
- DxMsgBroker: `broker-name={broker_type}`, `conn-info={broker_host}`, `topic={topic}`

### 2. Create Producer Pipeline Script

Create `dx_stream/pipelines/broker/run_broker_{broker_type}.sh`:
- Model auto-download
- Broker connection configuration
- Pipeline execution with `-e` flag

### 3. Create Consumer/Subscriber

For Kafka: Python script using `kafka-python`
For MQTT: Python script using `paho-mqtt`

### 4. Validate

```bash
# Verify broker is running
# Kafka: docker ps | grep kafka
# MQTT: systemctl status mosquitto

# Run producer pipeline
bash dx_stream/pipelines/broker/run_broker_{broker_type}.sh

# Run consumer (in separate terminal)
python3 consumer_{broker_type}.py {broker_host} {topic}
```

## Deliverables

- [ ] Producer pipeline script (run_broker_*.sh)
- [ ] Consumer/subscriber Python script
- [ ] Configuration files (broker config, msgconv config)
- [ ] Run instructions
