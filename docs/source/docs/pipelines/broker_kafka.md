## **Quick Guide for Kafka**

- **Install Kafka**
  ```
  $ sudo apt update
  $ sudo apt-get install default-jdk
  $ wget https://downloads.apache.org/kafka/3.9.0/kafka_2.13-3.9.0.tgz
  $ tar -xzf kafka_2.13-3.9.0.tgz
  $ cd kafka_2.13-3.9.0.tgz
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

For more information about Kafka, please refer to https://kafka.apache.org/ 

## **Secure connection**

To set up Kafka SSL, begin by generating and configuring SSL certificates for both the Kafka broker and the client. Follow the detailed instructions in the [librdkafka SSL guide](https://github.com/confluentinc/librdkafka/wiki/Using-SSL-with-librdkafka) to create and validate these certificates. Update the Kafka broker configuration file (server.properties) with the appropriate SSL settings, such as keystore, truststore paths, and passwords. Finally, configure the Kafka client with SSL options, including the paths to the keystore and truststore files, ensuring secure communication between the client and broker.



**Kafka server config**
- server.properties
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

**Kafka client(GstDxMsgBroker) config**

GstDxMsgBroker uses standard OpenSSL PEM keys. For instructions on how to key your client(GstDxMsgBroker), please refer to `broker_kafka.cfg` [here](./msgconv_broker.md)
