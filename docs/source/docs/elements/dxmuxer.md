
DxMuxer is an element required for multi-channel streaming. It receives video streams from multiple channels and pushes them to the next element one by one in a round-robin manner.

- In DxMuxer, `sink` pads are dynamically created according to the number of channels.
- The `chain` function is called each time a frame for a specific channel is delivered to the corresponding `sink` pad.

---

### **Key Features**

**Live Source Handling**

   - If `live-source` is set to `true`, buffers with timestamps older than 1000ms compared to the pipeline's running time are dropped in the `chain` function.
   - This ensures only the latest frames for each channel are processed, which is necessary for real-time video sources like RTSP.

**Merging Buffers**

   - When merging `N` buffers with different timestamps into a same timestamp, the timestamp is determined as follows
   - The merged buffer's timestamp is set to the **smallest timestamp** among the `N` buffers.
   - Channels with smaller timestamps update to the next buffer until their timestamps match or exceed the merged timestamp.
   - Synchronization is based on the channel with the **lowest FPS** for streams with varying FPS.

**QoS Event Handling**

   - Buffers may be dropped based on the type of QoS event received from downstream.

   - For **Underflow** events

      A positive `jitter` value indicates that the previous buffer arrived late.
      Buffers satisfying `B.timestamp < qos_timestamp + jitter` are dropped.


> **Info**
> - Multi-channel streaming with DxMuxer allows only sources of the same type.
> - It does not support streaming real-time sources (e.g., RTSP) together with non-real-time sources (e.g., video files).

---

### **Hierarchy**

```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----DxMuxer
```

---

### **Properties**

| **Name**         | **Description**                                                                                  | **Type**  | **Default Value** |
|-------------------|--------------------------------------------------------------------------------------------------|-----------|--------------------|
| `name`           | Sets the unique name of the DxMuxer element.                                                     | String    | `"dxmuxer0"`       |
| `live-source`    | Determines whether to allow only real-time video streams as input.                               | Boolean   | `false`            |







