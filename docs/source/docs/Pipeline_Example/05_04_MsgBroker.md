The pipeline demonstrates how to process a local video file with the YOLOv7 model for object detection, convert the results into a structured message format (e.g., JSON), and publish them to a message broker such as MQTT or Kafka for downstream applications.

![](./../../resources/05_04_msgbroker.png)

The pipeline in the figure is defined in 
`dx_stream/dx_stream/pipelines/broker/*.sh` and can be used as a reference for execution.

### **Explanation**

**Element Descriptions**  

- **`dxmsgconv`**: element that processes inference metadata from upstream **DxPostprocess** elements and converts it into message payloads in various formats.
- **`dxmsgbroker`**: sink element that transmits payload messages to an external message broker (e.g., MQTT, Kafka) 

### **Usage Notes**  

**Metadata Conversion**  
The `dxmsgconv` element requires a configuration file to define the message format. Update the `config-file-path` property to point to your message conversion configuration file.

**Message Publishing**  
The `dxmsgbroker` element requires the following properties.  

- **`conn-info`**: Connection information for the broker in the format `[host]:[port]`.  
- **`topic`**: The topic name for publishing the messages.

---

### **Broker Server Application**  

**MQTT**  

**MQTT** (Message Queuing Telemetry Transport) is a lightweight, publish/subscribe messaging protocol.

- **Install Mosquitto Server/Client**  

```
$ sudo apt update
$ sudo add-apt-repository ppa:mosquitto-dev/mosquitto-ppa
$ sudo apt install mosquitto mosquitto-clients
 
$ sudo systemctl status mosquitto
● mosquitto.service - Mosquitto MQTT Broker
     Loaded: loaded (/lib/systemd/system/mosquitto.service; enabled; vendor preset: enabled)
     Active: active (running) since Wed 2024-09-11 15:49:49 KST; 18min ago
       Docs: man:mosquitto.conf(5)
             man:mosquitto(8)
   Main PID: 19577 (mosquitto)
     Tasks: 1 (limit: 76827)
     Memory: 904.0K
     CGroup: /system.slice/mosquitto.service
        └─19577 /usr/sbin/mosquitto -c /etc/mosquitto/mosquitto.conf
```

- **CA certificates / Server keys, certificates**  

```
** Test Root CA
$ openssl genrsa -out ca.key 2048
$ openssl req -new -x509 -days 360 -key ca.key -out ca.crt -subj "/C=KR/ST=KK/L=SN/O=DXS/OU=Test/  CN=TestCA"
  
** Server Key and Certificate(Server's CN must match the host)
$ openssl genrsa -out server.key 2048
$ openssl req -new -out server.csr -key server.key -subj "/C=KR/ST=KK/L=SN/O=DXS/OU=Server/  CN=DXS-BROKER"
$ openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -days 360
$ openssl verify -CAfile ca.crt server.crt
  
$ ls -al
ca.crt  ca.key  ca.srl  server.crt  server.csr  server.key
  
$ cat /etc/mosquitto/mosquitto.conf
listener 8883
cafile /etc/mosquitto/ca_certificates/ca.crt
certfile /etc/mosquitto/certs/server.crt
keyfile /etc/mosquitto/certs/server.key
...
```

- **Id/Password**  

```
$ sudo mosquitto_passwd -c /etc/mosquitto/passwd user        ##create passwd file and add id
Password:
Reenter password:
$ sudo mosquitto_passwd -b /etc/mosquitto/passwd user1 1234  ##add id
$ sudo mosquitto_passwd -D /etc/mosquitto/passwd user1       ##remove id
$ sudo chmod 644 /etc/mosquitto/passwd
  
$ cat /etc/mosquitto/mosquitto.conf 
password_file /etc/mosquitto/passwd
allow_anonymous false
...
```

And for secure connection, you need to set the config file of dxmsgbroker as follows.  

- `broker_mqtt.cfg`

```
### username / password
#username = user
#password = 1234

### client-id, if not defined a random client idwill be generated
##client-id = client1

### enable ssl/tls encryption
tls_enable = 1

### the PEM encoded Certificate Authority certificates
#tls_cafile = <path to CA certificate file>

### the path to a file containing the CA certificates, 'openssl rehash <path to capath>' each time   you add/remove a certificate.
tls_capath = <path to directroy containing CA certificates>

### Path to the PEM encoded client certificate.
tls_certfile = <path to certificate file>

### Path to the PEM encoded client certificate.
tls_keyfile = <path to key file>
```

---

**Kafka**  

Apache **Kafka** is a distributed event streaming platform designed for high-throughput, fault-tolerant, and scalable handling of real-time data streams.  

- **Install Kafka**  

```
$ sudo apt update
$ sudo apt-get install default-jdk
$ wget https://downloads.apache.org/kafka/3.9.0/kafka_2.13-3.9.0.tgz
$ tar -xzf kafka_2.13-3.9.0.tgz
$ cd kafka_2.13-3.9.0
```

- **Zookeeper**  

```
$ bin/zookeeper-server-start.sh config/zookeeper.properties
```

- **Kafka server**  

```
$ bin/kafka-server-start.sh config/server.properties
```
  
- **Producer**  

```
$ bin/kafka-console-producer.sh --topic test-topic --bootstrap-server localhost:9092
```   
  
- **Consumer**  

```
$ bin/kafka-console-consumer.sh --topic test-topic --from-beginning --bootstrap-server localhost:9092
```   

For more information about Kafka, refer to [https://kafka.apache.org/](https://kafka.apache.org/)


- **Kafka Client( GstDxMsgBroker) Configuration**  

GstDxMsgBroker uses standard OpenSSL PEM keys.  

- **Secure Connection**  
 
To set up Kafka SSL, begin by generating and configuring SSL certificates for both the Kafka broker and the client. Follow the detailed instructions in the [librdkafka SSL guide](https://github.com/confluentinc/librdkafka/wiki/Using-SSL-with-librdkafka) to create and validate these certificates. Update the Kafka broker configuration file (server.properties) with the appropriate SSL settings, such as keystore, truststore paths, and passwords. Finally, configure the Kafka client with SSL options, including the paths to the keystore and truststore files, ensuring secure communication between the client and broker.  
  
- **Kafka Server Configuration**  

`server.properties`  

```
# SSL
ssl.protocol=TLS
ssl.enabled.protocols=TLSv1.2,TLSv1.1,TLSv1
ssl.keystore.type=JKS
ssl.keystore.location=<path to broker keystore file>
ssl.keystore.password=<broker keystore password>
ssl.key.password=<key password>
ssl.truststore.type=JKS
ssl.truststore.location=<path to broker truststore file>
ssl.truststore.password=<broker truststore password>
# To require authentication of clients use "require", else "none" or "request"
ssl.client.auth = required
```

And for secure connection, you need to set the config file of dxmsgbroker as follows.  

`broker_kafka.cfg`  

```
#######################################
### kafka client configuration
#######################################
[kafka]
### for frame data
message.max.bytes=10485760
  
### for secure transmission
#security.protocol=ssl
## CA certificate file for verifying the broker's certificate.
ssl.ca.location=<path to CA certificate file>
 
## Client's certificate
ssl.certificate.location=<path to client certificate file>

## Client's key
ssl.key.location=<path to client key file>

## Key password, if any
ssl.key.password=KEY_PASSWORD
```

---

**Script Descriptions**  
The server application, which receives data from the MQTT, KAFKA pipeline to log messages or render images, can be found in `/usr/share/dx-stream/bin`.    

- `mqtt_sub_example.py` (mqtt_sub_example)  

Runs an MQTT server that logs the messages received.  

```
python3 /usr/share/dx-stream/bin/mqtt_sub_example.py -p <PORT> -n <HOSTNAME> -t <TOPIC>
```

- `mqtt_sub_example_frame.py`  

Displays frames included in the messages using OpenCV.  

**Note.** The `include_frame` option in the DxMsgConv config JSON **must** be set to true for this feature to work.  

```
python3 /usr/share/dx-stream/bin/mqtt_sub_example_frame.py -p <PORT> -n <HOSTNAME> -t <TOPIC>
```

- `kafka_consume_example.py` (kafka_consume_example)  

Runs an KAFKA server that logs the messages received.  

```
python3 /usr/share/dx-stream/bin/kafka_consume_example.py <HOSTNAME> <TOPIC>
```

- `kafka_consume_example_frame.py`  

Displays frames included in the messages using OpenCV.  

**Note.** The include_frame option in the DxMsgConv config JSON **must** be set to true for this feature to work.  

```
python3 /usr/share/dx-stream/bin/kafka_consume_example_frame.py <HOSTNAME> <TOPIC>
```

---
