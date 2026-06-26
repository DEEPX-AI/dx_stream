# MQTT Subscriber Example

This example receives and displays JSON messages published by `dxmsgbroker` in a DX-Stream pipeline via MQTT.
When `dxmsgconv` is configured with `include-frame=true`, base64-encoded JPEG frame data in the JSON can be decoded and displayed in a window.

## Prerequisites

### Broker Service (Server Side)
An MQTT broker (Mosquitto) must be running.

```bash
# Install
sudo apt install mosquitto mosquitto-clients

# Start service
sudo systemctl start mosquitto
sudo systemctl enable mosquitto
```
Default port: **1883**

> **Remote access:** By default, Mosquitto only accepts connections from `localhost`.
> To allow remote consumers, add the following to `/etc/mosquitto/mosquitto.conf`:
> ```
> listener 1883 0.0.0.0
> allow_anonymous true
> ```
> Then restart the service: `sudo systemctl restart mosquitto`

### Producer Side (DX-Stream Pipeline)
The DX-Stream pipeline publishes messages via `dxmsgconv` + `dxmsgbroker`.

```bash
# Using the provided script
./pipelines/broker/run_dxmsgbroker_mqtt.sh

# Or run manually
gst-launch-1.0 \
  urisourcebin uri=file:///path/to/video.mp4 ! decodebin ! \
  dxpreprocess config-file-path=configs/.../preprocess_config.json ! \
  dxinfer config-file-path=configs/.../inference_config.json ! \
  dxpostprocess config-file-path=configs/.../postprocess_config.json ! \
  dxmsgconv library-file-path=/usr/lib/libdx_msgconvl.so include-frame=true ! \
  dxmsgbroker broker-name=mqtt conn-info=localhost:1883 topic=test
```

> Setting `include-frame=true` includes base64-encoded JPEG frame data in the JSON payload.

### Consumer Side (This Example App)
Only the **client libraries** are required — no broker service needed on the consumer machine.

#### C++ Build Dependencies
```bash
sudo apt install libmosquitto-dev libjson-glib-dev libopencv-dev
```

#### Python Dependencies
```bash
pip install paho-mqtt opencv-python
```

## Build (C++)

```bash
cd dx_stream/apps/mqtt
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
./mqtt_sub_example -n <broker_host> -t <topic>

# Print full JSON
./mqtt_sub_example -n <broker_host> -t <topic> -a

# Display frames in a window
./mqtt_sub_example -n <broker_host> -t <topic> -d
```

### Python
```bash
# Summary output (default)
python mqtt_sub_example.py -n <broker_host> -t <topic>

# Print full JSON
python mqtt_sub_example.py -n <broker_host> -t <topic> -a

# Display frames in a window
python mqtt_sub_example.py -n <broker_host> -t <topic> -d
```

## Options

| Option | C++ | Python | Description |
|--------|-----|--------|-------------|
| Broker host | `-n <host>` | `-n <host>` | MQTT broker address |
| Topic | `-t <topic>` | `-t <topic>` | MQTT topic to subscribe to |
| Port | `-p <port>` | `-p <port>` | Broker port (default: 1883) |
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
