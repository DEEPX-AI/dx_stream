#!/usr/bin/env python

import json
import threading
import argparse
import time
import base64
import cv2
import numpy as np

import paho.mqtt.client as mqtt

class CircularBuffer:
    def __init__(self, size):
        self.size = size
        self.buffer = [None] * size
        self.prod_ref_index = 0
        self.cons_ref_index = 0
        self.lock = threading.Lock()

    def isEmpty(self):
        with self.lock:
            return self.prod_ref_index == self.cons_ref_index

    def enQueue(self, data):
        with self.lock:
            next_index = (self.prod_ref_index + 1) % self.size
            if next_index == self.cons_ref_index:
                return False
            self.buffer[self.prod_ref_index] = data
            self.prod_ref_index = next_index
            return True

    def deQueue(self):
        with self.lock:
            if self.prod_ref_index == self.cons_ref_index:
                return None
            data = self.buffer[self.cons_ref_index]
            self.cons_ref_index = (self.cons_ref_index + 1) % self.size
            return data

    def isAvailable(self):
        with self.lock:
            if self.prod_ref_index >= self.cons_ref_index:
                return self.prod_ref_index - self.cons_ref_index
            return self.size - self.cons_ref_index + self.prod_ref_index

def on_connect(client, userdata, flags, rc):
    print(f"Connected with result code {rc}")
    client.subscribe(userdata['topic'])

def on_message(client, userdata, msg):
    message = json.loads(msg.payload)
    
    while True:
        ret = buffer.enQueue(message)
        if ret:
            print(f"Enqueue message, seqId: {message.get('seqId')}")
            break
        else:
            print(f"    ~~~ waiting enqueue, seqId: {message.get('seqId')}")
            time.sleep(0.1)
            continue

def process_message_thread(userdata):
    buffer = userdata['buffer']
    while True:
        if buffer.isEmpty():
            time.sleep(0.01)
            continue

        message = buffer.deQueue()
        if not message:
            time.sleep(0.01)
            continue

        available = buffer.isAvailable()
        seq_id = message.get('seqId')

        if available >= 5:
            print(f"    ~~~ Skip process, seqId: {seq_id}, Available: {available}")
            time.sleep(0.01)
            continue

        print(f"Process message, seqId: {seq_id}, Available: {available}")

        frame_data_b64 = message.get('frameData')
        if not frame_data_b64:
            time.sleep(0.01)
            continue

        frame_data = base64.b64decode(frame_data_b64)
        np_arr = np.frombuffer(frame_data, np.uint8)
        img = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)

        if img is None:
            time.sleep(0.01)
            continue

        img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        img = cv2.resize(img, (userdata['width'], userdata['height']))
        cv2.imshow('Slideshow', img)
        cv2.waitKey(1)

        time.sleep(0.01)



if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--hostname", required=True, help="MQTT server hostname")
    parser.add_argument("--topic", required=True, help="MQTT topic to subscribe to")
    parser.add_argument("--port", type=int, default=1883, help="MQTT server port")
    args = parser.parse_args()

    buffer = CircularBuffer(30)
    sub_ts_old = time.time()

    # display width and height
    display_width=1280
    display_height=720
    #display_width=1920
    #display_height=1080

    # Create a named window
    # cv2.namedWindow("Slideshow", cv2.WINDOW_NORMAL)
    # cv2.resizeWindow("Slideshow", display_width, display_height)
    # cv2.moveWindow('Slideshow', 100, 100)  # Adjust the position

    client = mqtt.Client(userdata={'topic': args.topic, 'sub_ts_old': sub_ts_old})
    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(args.hostname, args.port, 60)

    thread = threading.Thread(target=process_message_thread, args=({'topic': args.topic, 'sub_ts_old': sub_ts_old, 'buffer': buffer, 'width': display_width, 'height': display_height},))
    thread.start()

    client.loop_forever()