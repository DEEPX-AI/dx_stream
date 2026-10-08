#!/bin/bash
# Raspberry Pi camera detection shared by run_demo.sh and scripts/setup/08_rpi_camera.sh.

is_raspberry_pi() {
    tr -d '\0' < /proc/device-tree/model 2>/dev/null | grep -q "^Raspberry Pi"
}

has_libcamerasrc() {
    gst-inspect-1.0 libcamerasrc >/dev/null 2>&1
}

# CSI camera only: USB webcams are also listed by libcamera but rarely provide the NV12 mode the demos request.
# Lists cameras without opening them; falls back to grabbing one frame when rpicam-apps is absent.
has_rpi_camera() {
    if command -v rpicam-hello >/dev/null 2>&1; then
        rpicam-hello --list-cameras 2>/dev/null | grep -E "^[0-9]+ : " | grep -qv "usb"
        return
    fi
    timeout 5 gst-launch-1.0 -q libcamerasrc num-buffers=1 ! fakesink >/dev/null 2>&1
}
