#!/usr/bin/env python
#
# pip install paho-mqtt

import json

import argparse
import paho.mqtt.client as mqtt
import time


'''
/*** Payload format: json
 *
 * {
 *   "streamId" : 0,
 *   "seqId" : 1,
 *   "width" : 1920,
 *   "height" : 1080,
 *   "customId" : 999,
 *   "objects" : [
 *     {
 *         OBJECT_TYPE: { ... }
 *     },
 *     ...
 *   ],
 *   "segment" : {
 *     "height" : 384,
 *     "width" : 768,
 *     "data" : 140040227284784
 *   }
 * }
 *
 *
 *** OBJECT_TYPE: "object" | "track" |  "face" | "pose"
 *
 *       "object" : {
 *         "id" : 0,
 *         "confidence" : 0.7830657958984375,
 *         "name" : "face",
 *         "box" : {
 *           "startX" : 828,
 *           "startY" : 271,
 *           "endX" : 887,
 *           "endY" : 350
 *         }
 *       }
 *
 *       "track" : {
 *         "id" : 1,
 *         "box" : {
 *           "startX" : 320,
 *           "startY" : 232,
 *           "endX" : 639,
 *           "endY" : 926
 *         }
 *       }
 *
 *       "face" : {
 *         "landmark" : [
 *           {
 *             "x" : 838,
 *             "y" : 299
 *           },
 *           ...
 *       }
 *
 *       "pose" : {
 *         "keypoints" : [
 *           {
 *             "kx" : 118,
 *             "ky" : 272,
 *             "ks" : 0.92519611120223999
 *           },
 *           ...
 *         ]
 *       },
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
