#!/usr/bin/env python
#
# pip install paho-mqtt

import json

import argparse
import paho.mqtt.client as mqtt
import time


'''
/*
 * Sample JSON Output Format:
 * {
 *   "streamId": 0,
 *   "seqId": 123,
 *   "width": 1920,
 *   "height": 1080,
 *   "objects": [
 *     {
 *       "object": {
 *         "label_id": 1,
 *         "track_id": 42,
 *         "confidence": 0.87,
 *         "name": "person",
 *         "box": {
 *           "startX": 300.0,
 *           "startY": 400.0,
 *           "endX": 500.0,
 *           "endY": 600.0
 *         },
 *         "body_feature": [0.321, 0.654, 0.987],
 *         "segment": {
 *           "height": 1080,
 *           "width": 1920,
 *           "data": 140712345678912
 *         },
 *         "pose": {
 *           "keypoints": [
 *             {"kx": 100.5, "ky": 200.3, "ks": 0.8},
 *             {"kx": 105.2, "ky": 205.7, "ks": 0.9}
 *           ]
 *         },
 *         "face": {
 *           "landmark": [
 *             {"x": 150.2, "y": 180.5},
 *             {"x": 155.8, "y": 185.3}
 *           ],
 *           "box": {
 *             "startX": 100.0,
 *             "startY": 150.0,
 *             "endX": 200.0,
 *             "endY": 250.0
 *           },
 *           "confidence": 0.95,
 *           "face_feature": [0.123, 0.456, 0.789]
 *         }
 *       }
 *     }
 *   ]
 * }
 */
'''


# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))
    # Subscribing to the topic
    client.subscribe(userdata['topic'])

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    try:
        # Decode the message payload and parse it as JSON
        message = json.loads(msg.payload.decode())
        ###print("Received message: ", message)

        sub_ts = int(round(time.time() * 1000))
        seqId = message['seqId'] if 'seqId' in message else None

        if seqId is None:
            return

        if seqId==1:
            userdata['sub_ts_old']=sub_ts

        ## print payload
        print(f"|dSub: {(sub_ts - userdata['sub_ts_old']):3d}| payload => seqId: {seqId:3d}, ...")

        userdata['sub_ts_old']=sub_ts

    except json.JSONDecodeError:
        print("Received non-JSON message: ", msg.payload.decode())
   

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='MQTT Subscriber')
    parser.add_argument('-n', '--hostname', type=str, required=True, help='MQTT broker hostname')
    parser.add_argument('-t', '--topic', type=str, required=True, help='MQTT topic to subscribe to')
    parser.add_argument('-p', '--port', type=int, default=1883, help='MQTT broker port number (default: 1883)')
    
    args = parser.parse_args()
    
    # old publish/subscribe timestamp
    sub_ts_old=0
    
    client = mqtt.Client()
    client.user_data_set({'topic': args.topic, 'sub_ts_old': sub_ts_old})
    client.on_connect = on_connect
    client.on_message = on_message
    
    client.connect(args.hostname, args.port, 60)
    
    client.loop_forever()
   
    cv2.destroyAllWindows()
