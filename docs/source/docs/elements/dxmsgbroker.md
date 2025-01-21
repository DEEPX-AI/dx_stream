
DxMsgBroker is a sink element that sends payload messages to a message server, allowing them to be published to a broker (e.g., MQTT, Kafka).

- It is placed after the `DxMsgConv` element in the pipeline and processes the payload messages generated upstream.
- Connection to the message server (e.g., MQTT broker) is established using the `conn-info` property.

---

### **Overview**

**Message Sending**

- Processes payloads received from upstream elements and publishes them to the configured broker server.
- Compatible with brokers such as MQTT.

**Connection Information**

- The `conn-info` property specifies the connection details in the format `host:port`.
- An optional `config` file can provide additional connection details, such as SSL/TLS settings.

**Pipeline Integration**

- Used as a sink element, typically following `DxMsgConv`.

---
### **Hierarchy**

```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstBaseSink
                         +----GstDxMsgBroker
```

---
### **Properties**

| **Name**         | **Description**                                                      | **Type**  | **Default Value** |
|-------------------|----------------------------------------------------------------------|-----------|--------------------|
| `broker-name` | Name of message broker system (mqtt or kafka). **Required**. | String | `mqtt`             |
| `conn-info`      | Connection info in the format `host:port`. **Required**.             | String    | `null`             |
| `topic`          | The topic name for publishing messages. **Required**.                | String    | `null`             |
| `config`         | Path to the broker configuration file (optional).                    | String    | `null`             |

---

### **Notes**
- The `broker-name` property is for selecting a broker system(MQTT, Kafka).
- The `conn-info` property is mandatory for establishing a connection to the broker.
- The `topic` property is mandatory for topic for the message.
- The `config` file is optional and allows for additional parameters, such as SSL/TLS encryption and authentication details.
- Ensure `libmosquitto-dev` and `librdkafka` are installed for proper functionality.
