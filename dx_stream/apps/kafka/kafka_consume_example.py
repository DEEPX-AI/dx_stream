#!/usr/bin/env python
#
# pip install confluent-kafka opencv-python

import signal
import json
import argparse
import base64

from confluent_kafka import Consumer, KafkaError

try:
    import cv2
    import numpy as np
    HAS_CV2 = True
except ImportError:
    HAS_CV2 = False

run = True


def sigterm_handler(signum, frame):
    global run
    run = False


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


def parse_message(msg, config):
    raw = msg.value()
    payload = raw.decode('utf-8')

    try:
        data = json.loads(payload)
    except json.JSONDecodeError as e:
        print(f"Unable to parse JSON: {e}")
        return

    seq_id = data.get('seqId', -1)
    objects = data.get('objects', [])
    frame_data = data.get('frameData', '')
    has_frame = len(frame_data) > 0 if frame_data else False

    print(f"Received payload {len(raw)} bytes | seqId: {seq_id} | "
          f"objects: {len(objects)} | frameData: {'yes' if has_frame else 'no'}")

    if config.get('print_all'):
        display = dict(data)
        if has_frame:
            display['frameData'] = '<base64 omitted>'
        print(json.dumps(display, indent=2))

    if has_frame and config.get('display'):
        display_frame(frame_data)


def main(broker, topic, config):
    conf = {
        'bootstrap.servers': broker,
        'group.id': 'my-group',
        'auto.offset.reset': 'earliest'
    }

    consumer = Consumer(conf)

    def print_assignment(consumer, partitions):
        print('Assignment:', partitions)

    consumer.subscribe([topic], on_assign=print_assignment)

    signal.signal(signal.SIGINT, sigterm_handler)
    signal.signal(signal.SIGTERM, sigterm_handler)

    try:
        while run:
            msg = consumer.poll(timeout=1.0)
            if msg is None:
                continue
            if msg.error():
                if msg.error().code() == KafkaError._PARTITION_EOF:
                    continue
                else:
                    print(f"Failed to consume message: {msg.error()}")
                    continue

            parse_message(msg, config)
    except KeyboardInterrupt:
        pass
    finally:
        consumer.close()


def parse_args():
    parser = argparse.ArgumentParser(description='Kafka message consumer')
    parser.add_argument('-n', '--hostname', required=True,
                        help='Kafka broker hostname (e.g., localhost)')
    parser.add_argument('-p', '--port', type=int, default=9092,
                        help='Kafka broker port (default: 9092)')
    parser.add_argument('-t', '--topic', required=True,
                        help='Topic name to subscribe to')
    parser.add_argument('-a', '--all', action='store_true',
                        help='Print all JSON fields')
    parser.add_argument('-d', '--display', action='store_true',
                        help='Display decoded JPEG frames in a window')
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    broker = f"{args.hostname}:{args.port}"

    config = {
        'print_all': args.all,
        'display': args.display,
    }

    main(broker, args.topic, config)
