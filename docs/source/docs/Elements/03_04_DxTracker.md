**DxTracker** is a GStreamer element designed to perform Multi-Object Tracking (MOT) on object detection results in a video stream. It uses bounding box metadata from upstream detection elements (such as **DxPostprocess**) to continuously track and assign consistent IDs across frames.   

Currently, **DxTracker** supports the `OC_SORT` algorithm as its default tracking method. For more information about `OC_SORT`, refer to [the Object Tracking Algorithms](https://github.com/noahcao/OC_SORT) document. 


### **Key Features**

**Multi-Object Tracking (MOT)**  
**DxTracker** tracks the movement of detected objects over time. It assigns unique track IDs to each object. Objects that can **not** be reliably tracked are discarded and **not** forwarded downstream.  

**Configurable Tracking Algorithms**  
Tracking algorithms and their parameters are defined in a JSON configuration file specified via the `config-file-path` property.  

Example JSON configuration
```json
{
    "tracker_name": "OC_SORT",
    "params": {
        "det_thresh": 0.5,
        "max_age": 30,
        "min_hits": 3,
        "iou_threshold": 0.3,
        "delta_t": 3,
        "asso_func": "iou",
        "inertia": 0.2,
        "use_byte": false
    }
}
```

The configuration file must include  

- `tracker_name`: Specifies the tracking algorithm (e.g., `OC_SORT`)  
- `params`: Contains algorithm-specific parameters

**Default Behavior**  
If no config file is provided, **DxTracker** uses the `OC_SORT`algorithm with built-in default values.  
If only `tracker_name` is specified without params, the `default` parameter values for that algorithm are applied.

**Limitations**  
The number of tracked objects is always less than or equal to the number of detected bounding boxes.  
Algorithm parameters **must** be defined within the `params` block of the JSON file. These can **not** be set as individual GStreamer properties on the element.

### **Hierarchy**

```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstBaseTransform
                         +----DxTracker
```

### **Properties**

| **Name**            | **Description**                                                                                      | **Type**  | **Default Value** |
|----------------------|------------------------------------------------------------------------------------------------------|-----------|--------------------|
| `name`              | Sets the unique name of the DxTracker element.                                                      | String    | `"dxtracker0"`     |
| `config-file-path`  | Path to the JSON config file containing the tracking algorithm and parameters.                       | String    | `null`             |
| `tracker-name`      | Specifies the name of the tracking algorithm to use.                                                | String    | `"OC_SORT"`        |

**Notes.**  

- The JSON configuration file is required to customize tracking algorithm parameters.  
- If **no** configuration file is provided, **DxTracker** uses `OC_SORT` with default settings.  
- All parameters values **must** be defined within the params section of the JSON file.  
- Track IDs are only assigned to successfully tracked objects. Objects that can **not** be tracked are removed and **not** passed downstream.  

---
