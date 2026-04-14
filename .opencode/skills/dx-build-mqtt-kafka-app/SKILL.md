---
name: dx-build-mqtt-kafka-app
description: Build MQTT/Kafka message broker pipeline for dx_stream with DxMsgConv and DxMsgBroker elements
---

# Build MQTT/Kafka Pipeline

Read `.deepx/skills/dx-build-mqtt-kafka-app.md` for full patterns.

## Quick Reference
- DxMsgConv serializes inference results to JSON/protobuf
- DxMsgBroker publishes to MQTT or Kafka topic
- Always: DxMsgConv BEFORE DxMsgBroker in the pipeline
