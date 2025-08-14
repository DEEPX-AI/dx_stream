This chapter describes the installation of **DX-STREAM** in both source-based and Docker-based environments.

## System Requirements

This section describes the hardware and software requirements for running **DX-STREAM**.

**Hardware and Software Requirements**  

- **CPU:** aarch64, x86_64
- **RAM:** 8GB RAM (16GB RAM or higher is recommended)
- **Storage:** 4GB or higher available disk space
- **OS:** Ubuntu 18.04 / 20.04 / 22.04 / 24.04 (x64 / aarch64)
- **DX-RT must** be installed 3.0.0 or higher available 
- The system **must** support connection to an **M1 M.2** module with the M.2 interface on the host PC. 

![](./../resources/02_DX-M1_M.2_LPDDR5x2_PCP.png){ width=700px }


**Note.** The **NPU Device Driver** and **DX-RT must** be installed. Refer to **DX-RT User Manual** for detailed instructions on installing NPU Device Driver and DX-RT. 

---

## Install DX-STREAM

**DX-STREAM** can be installed either by building from source or by using a pre-built Docker image (please refer to DX-AS Docs). This section explains both approaches.

### Build from Sources

**1.** Unzip the Package  

Extract the **DX-STREAM** source files.  
```
$ unzip dxstream_vX.X.X.zip -d dx_stream
$ cd dx_stream
```

**2.** Install Dependencies  

Run the provided script to automatically install all required packages.  
```
$ ./install.sh
```

**3.** Build **DX-STREAM**  

Compile **DX-STREAM**.  
```
$ ./build.sh
```

(Optional) Build with debug symbols.  
```
$ ./build.sh --debug
```

**4.** Verify the installation  

Check that the plugin is correctly installed.  
```
$ gst-inspect-1.0 dxstream
```

**Note.** If you want to remove **DX-STREAM**, use the following command.  
```
$ ./build.sh --uninstall
```

## Run DX-STREAM

This section provides a step-by-step guide of quickly running **DX-STREAM**'s sample pipelines

**Requirements**  
Before you start, ensure the following prerequisites are met.  

- Properly install **DX-RT**, **the NPU Device Driver**, and **DX-STREAM** in the correct order.
- Download the sample video and model needed to run the demo.

```
$ cd dx_stream
$ ./setup.sh
```

By running the above command, you can download the resources needed for the demo.


**Run Demo Pipelines**  

Execute the demo script.
```
$ cd dx_stream
$ ./run_demo.sh
```

When the script is executed, you'll be prompted to select a demo from the following options.
```
0: Object Detection (YOLOv7)
1: Face Detection (YOLOV5S_Face)
2: Multi-Object Tracking
3: Pose Estimation
4: Semantic Segmentation
5: Multi-Channel Object Detection
6: Multi-Channel RTSP
7: secondary mode
which AI demo do you want to run:(timeout:10s, default:0)
```

Enter the number corresponding to the desired demo to run it.  

If **no** input is provided within 10 seconds, the default option (`0: Object Detection`) will be executed automatically.  

**Notes.** Each demo corresponds to a specific pipeline described in **Chapter 5. Pipeline Examples**.

---
