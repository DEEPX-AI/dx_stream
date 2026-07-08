#!/usr/bin/env python
#
# pip install paho-mqtt opencv-python

import json
import argparse
import base64

import paho.mqtt.client as mqtt

try:
    import cv2
    import numpy as np
    HAS_CV2 = True
except ImportError:
    HAS_CV2 = False


def display_frame(base64_str):
    if not HAS_CV2:
        print("  -> opencv-python not installed, skipping frame display")
        return

    decoded = base64.b64decode(base64_str)
    nparr = np.frombuffer(decoded, np.uint8)
    img = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
    if img is None:
        print("  -> Failed to decode JPEG from frameData")
        return

    cv2.imshow("frameData", img)
    cv2.waitKey(1)


def on_connect(client, userdata, flags, rc):
    print(f"on_connect: {rc}")
    client.subscribe(userdata['topic'])


def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode())
    except json.JSONDecodeError:
        print(f"Unable to parse JSON: {msg.payload.decode()[:100]}")
        return

    seq_id = data.get('seqId', -1)
    objects = data.get('objects', [])
    frame_data = data.get('frameData', '')
    has_frame = len(frame_data) > 0 if frame_data else False

    print(f"Received payload {len(msg.payload)} bytes | seqId: {seq_id} | "
          f"objects: {len(objects)} | frameData: {'yes' if has_frame else 'no'}")

    if userdata.get('print_all'):
        display = dict(data)
        if has_frame:
            display['frameData'] = '<base64 omitted>'
        print(json.dumps(display, indent=2))

    if has_frame and userdata.get('display'):
        display_frame(frame_data)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='MQTT Subscriber')
    parser.add_argument('-n', '--hostname', type=str, required=True,
                        help='MQTT broker hostname')
    parser.add_argument('-t', '--topic', type=str, required=True,
                        help='MQTT topic to subscribe to')
    parser.add_argument('-p', '--port', type=int, default=1883,
                        help='MQTT broker port (default: 1883)')
    parser.add_argument('-a', '--all', action='store_true',
                        help='Print all JSON fields')
    parser.add_argument('-d', '--display', action='store_true',
                        help='Display decoded JPEG frames in a window')

    args = parser.parse_args()

    client = mqtt.Client()
    client.user_data_set({
        'topic': args.topic,
        'print_all': args.all,
        'display': args.display,
    })
    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(args.hostname, args.port, 60)
    client.loop_forever()
