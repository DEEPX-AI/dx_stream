#!/usr/bin/env python


import signal
import sys
import json
import threading
from confluent_kafka import Consumer, KafkaException, KafkaError
import time
import base64
import cv2
import numpy as np

run = True

###
### CircularBuffer class
###
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
    
    def getSize(self):
        return self.size

###
### Signal handler
###
def sigterm_handler(signum, frame):
    global run
    run = False

###
### on message
###
def on_message(msg, buffer):
    payload = msg.value().decode('utf-8')

    try:
        data = json.loads(payload)
    except json.JSONDecodeError as e:
        print(f"Unable to parse JSON: {e}")
        return

    if 'seqId' in data:
        seqId = data['seqId']
        recvString=f'Received {len(payload)} bytes, seqId: {seqId}'
    else:
        recvString=f'Received {len(payload)} bytes, unknown seqId'

    if buffer.enQueue(data):
        print(f"Enqueued message: {recvString}")
    else:
        print(f"~~~ Buffer full, unable to enqueue message: {recvString}")

###
### Process message thread
###
def process_message_thread(buffer, userdata):
    while run:
        if not buffer.isEmpty():
            message = buffer.deQueue()
            if message:
                if buffer.isAvailable() < buffer.getSize()/2:
                    print(f"Process message, seqId: {message.get('seqId')}, Available: {buffer.isAvailable()}")
                    ###print("Received message: ", message)
                    if 'frameData' in message and message['frameData']:
                        ##print("Received message: ", message)
                        # Decode the base64 encoded frame data
                        frame_data = base64.b64decode(message['frameData'])
                        # Convert the bytes to a numpy array
                        np_arr = np.frombuffer(frame_data, np.uint8)
                        # Decode the numpy array to an image
                        img = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)

                        # color space BGR to RGB 
                        img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
                        '''
                        # Save the image to a file
                        if seqId is not None:
                            filename = f"./images/image_{int(seqId):03d}.jpg"
                        else:
                            filename = "./images/image_xxx.jpg"
                        cv2.imwrite(filename, img)
                        '''

                        # Display the image using OpenCV
                        img = cv2.resize(img, (userdata['width'], userdata['height']))
                        cv2.imshow('Slideshow', img)
                        cv2.waitKey(1)

                else:
                    print(f"    ~~~ Skip process, seqId: {message.get('seqId')}, Available: {buffer.isAvailable()}")
        time.sleep(0.001)


###
### main
###
def main(broker, topic):
    conf = {
        'bootstrap.servers': broker,
        'group.id': 'my-group',
        'auto.offset.reset': 'latest'
    }

    consumer = Consumer(conf)
    buffer = CircularBuffer(50)

    sub_ts_old = time.time()

    # display width and height
    display_width=1280
    display_height=720
    #display_width=1920
    #display_height=1080

    userdata = {'topic': topic, 'sub_ts_old': None, 'width': display_width, 'height': display_height}

    def print_assignment(consumer, partitions):
        print('Assignment:', partitions)

    consumer.subscribe([topic], on_assign=print_assignment)

    signal.signal(signal.SIGINT, sigterm_handler)
    signal.signal(signal.SIGTERM, sigterm_handler)

    threading.Thread(target=process_message_thread, args=(buffer, userdata)).start()

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

            on_message(msg, buffer)
            time.sleep(0.001)
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
