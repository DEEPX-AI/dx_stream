**DxMsgBroker** is a sink element that transmits payload messages to an external message broker (e.g., MQTT, Kafka)  
It is typically placed after the **DxMsgConv** element in the pipeline and is responsible for delivering the formatted messages to the configured broker server.  
Connection settings are managed using the `conn-info` property. 

### **Key Features**

**Message Sending**  

- Receives payloads from upstream elements and publishes them to the selected broker.  
- Compatible with broker systems such as MQTT and Kafka.  
- If the broker is not connected or publishing fails, the message is dropped with a warning. After 10 consecutive failures, the element posts an error and the pipeline stops. A successful publish resets the counter.  

**Connection Information**  

- The `conn-info` property specifies the connection in the format `host:port`.  
- Additional connection details (e.g., `SSL/TLS` settings) can be specified through an optional configuration file.  

**Pipeline Integration**  

- Used as a sink element, typically placed just after **DxMsgConv** in the pipeline.  

### **Hierarchy**

```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstBaseSink
                         +----GstDxMsgBroker
```

### **Pad Templates**

**Sink (input)**

| **Property** | **Value** |
|---|---|
| Format | `video/x-raw; application/x-dxvideoraw` |

`DxMsgBroker` is caps-agnostic — it only reads message payloads from buffer metadata produced by `DxMsgConv`, so it accepts both single-stream `video/x-raw` and the multi-stream `application/x-dxvideoraw` domain caps.

### **Properties**

| **Name**         | **Description**                                                      | **Type**  | **Default Value** |
|-------------------|----------------------------------------------------------------------|-----------|--------------------|
| `broker-name` | Name of message broker system (mqtt or kafka). **Required**. | String | `"mqtt"`             |
| `conn-info`      | Connection info in the format `host:port`. **Required**.             | String    | `null`             |
| `topic`          | The topic name for publishing messages. **Required**.                | String    | `null`             |
| `config`         | Path to the broker configuration file (optional).                    | String    | `null`             |


!!! note "NOTE" 

    - The `broker-name` property determines which broker type to use. Supported values are `"mqtt"` and `"kafka"`. While "mqtt" is set as the initial value, this property is **required** to be explicitly set.
    - The `conn-info` property is **required** and **must** be set to establish a connection to the broker in the `host:port` format.  
    - The `topic` property is **required** and defines the topic for message publication.  
    - An optional configuration file can provide advanced settings, such as `SSL/TLS` encryption and authentication details.  
    - Ensure the `libmosquitto-dev` (for MQTT) and `librdkafka-dev` (for Kafka) libraries are installed for proper broker support.  
    - The element uses a **Broker Abstraction Layer (BAL)** that provides a unified interface for different broker types, making it easy to switch between MQTT and Kafka without changing the pipeline structure.  

!!! warning "Property validation"

    `broker-name`, `conn-info`, and `topic` are validated when the element starts. If `broker-name` is not `mqtt` or `kafka`, or if `conn-info` or `topic` is empty, the element posts a `GST_ELEMENT_ERROR` and fails to start. Always set `conn-info` and `topic` explicitly.

---
