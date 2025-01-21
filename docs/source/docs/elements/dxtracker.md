
DxTracker is an element designed to perform Multi-Object Tracking (MOT) on object detection results in a video stream. 

- DxTracker uses bounding box information detected by upstream elements to track and identify multiple objects over time.
- Currently, DxTracker supports the **OC_SORT** algorithm as the default tracking algorithm. For details on OC_SORT, refer to the relevant documentation.

---

### **Key Features**
**Multi-Object Tracking (MOT)**

- Tracks the movement of detected objects over time and assigns unique IDs (Track IDs) to them.
- Objects that cannot be successfully tracked are discarded and not forwarded downstream.

**Configurable Tracking Algorithms**

- Tracking algorithms and their parameters are configured using a JSON config file specified via the `config-file-path` property.
- Example JSON configuration:
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
- **tracker_name**: Specifies the tracking algorithm to use.
- **params**: Contains the algorithm-specific parameters.

**Default Behavior**

- If no config file is provided, DxTracker uses the OC_SORT algorithm with default parameter values.
- If only `tracker_name` is specified without `params`, default parameter values for the specified algorithm are used.

**Limitations**

- The number of tracked objects is always less than or equal to the number of detected bounding boxes.
- Algorithm-specific parameters must be defined within the `params` section of the JSON file and cannot be set as individual properties.

---

### **Hierarchy**
```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstBaseTransform
                         +----DxTracker
```

---

### **Properties**

| **Name**            | **Description**                                                                                      | **Type**  | **Default Value** |
|----------------------|------------------------------------------------------------------------------------------------------|-----------|--------------------|
| `name`              | Sets the unique name of the DxTracker element.                                                      | String    | `"dxtracker0"`     |
| `config-file-path`  | Path to the JSON config file containing the tracking algorithm and parameters.                       | String    | `null`             |
| `tracker-name`      | Specifies the name of the tracking algorithm to use.                                                | String    | `"OC_SORT"`        |

---

### **Notes**
The JSON config file is mandatory for setting algorithm parameters. Without it, only the default OC_SORT algorithm will be used.
Parameters must be defined within the params section of the JSON file.
Track IDs are only assigned to successfully tracked objects; untracked objects are removed and not sent downstream.