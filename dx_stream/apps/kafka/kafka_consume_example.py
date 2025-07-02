#!/usr/bin/env python

import signal
import sys
import json
from confluent_kafka import Consumer, KafkaException, KafkaError

run = True

def sigterm_handler(signum, frame):
    global run
    run = False

def parse_message(msg, bPrintAll):
    payload = msg.value().decode('utf-8')

    try:
        data = json.loads(payload)
    except json.JSONDecodeError as e:
        print(f"Unable to parse JSON: {e}")
        return

    if bPrintAll:
        print(f"Received payload {len(payload)} bytes, (All): {json.dumps(data, indent=2)}")
    else:
        if 'seqId' in data:
            seqId = data['seqId']
            print(f"Received payload {len(payload)} bytes, seqId: {seqId}")
        else:
            print("Received payload is not a JSON object or does not contain 'seqId'.")

def main(broker, topic):
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

            parse_message(msg, bPrintAll=False)
    except KeyboardInterrupt:
        pass
    finally:
        consumer.close()

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <broker> <topic>")
        sys.exit(1)

    broker = sys.argv[1]
    topic = sys.argv[2]

    main(broker, topic)
