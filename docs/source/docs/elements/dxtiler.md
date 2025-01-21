
DxTiler is an element designed for multi-channel streaming, arranging video frames from multiple channels into a grid layout to form a single large frame.

- The element combines frames from multiple channels into a tiled grid for simultaneous display.
- Frames are arranged in a grid specified by the `rows` and `cols` properties.
- The dimensions of each tile are set using the `width` and `height` properties, determining the size of the resulting tiled frame.

---

### **Key Features**
**Grid Configuration**

- Frames are arranged in a grid with dimensions specified by `rows` (number of rows) and `cols` (number of columns).
- The number of channels must not exceed `rows x cols`. If it does, an error message is generated.

**Tiling Dimensions**

- Each tile's size is determined by `width` and `height`.
- The overall frame size is calculated as

    **Width**: `width x cols`
    
    **Height**: `height x rows`

**Configuration via JSON**

- Properties can be specified using a JSON config file. Example:
    ```json
    {
        "width": 640,
        "height": 480,
        "cols": 3,
        "rows": 4
    }
    ```

**Placement in Pipeline**

- `DxTiler` must be placed immediately before the sink element in the pipeline.

---

### **Hierarchy**

```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----DxTiler
```

---

### **Properties**

| **Name**            | **Description**                                                    | **Type**  | **Default Value** |
|----------------------|--------------------------------------------------------------------|-----------|--------------------|
| `name`              | Sets the unique name of the DxTiler element.                      | String    | `"dxtiler0"`       |
| `config-file-path`  | Path to the JSON config file containing the element's properties.  | String    | `null`             |
| `rows`              | Sets the number of rows in the grid.                              | Integer   | `1`                |
| `cols`              | Sets the number of columns in the grid.                           | Integer   | `1`                |
| `width`             | Sets the width of each tile in the grid.                          | Integer   | `1920`             |
| `height`            | Sets the height of each tile in the grid.                         | Integer   | `1080`             |

---

### **Example JSON Configuration**

```json
{
    "width": 640,
    "height": 480,
    "cols": 3,
    "rows": 4
}
```

---

### **Notes**
- Ensure that the number of channels does not exceed rows x cols to avoid errors.
- Place DxTiler immediately before the sink element in the pipeline for proper operation.
- The tiled frame's dimensions are determined by the properties:
    - Width: width x cols
    - Height: height x rows
