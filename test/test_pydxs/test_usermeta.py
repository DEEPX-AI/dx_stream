#!/usr/bin/env python3
"""
DX-Stream User Metadata Test (Python)

Python version of C++ test_usermeta.cpp - same level of testing.
Like the C++ version, this test does NOT require external video files.
It uses videotestsrc to generate test frames, focusing on metadata API testing.

Tests basic pydxs functionality:
1. Create DXFrameMeta
2. Add user metadata to frames
3. Create and add DXObjectMeta to frames
4. Add user metadata to objects
5. Read and validate all metadata

Usage:
    python test_usermeta.py
"""

import sys
import random
import pydxs

try:
    import gi
except ImportError:
    import os
    print("ERROR: gi module (python3-gi) not found!")
    print("")
    
    # Check if running in virtual environment
    venv_path = os.environ.get('VIRTUAL_ENV')
    if venv_path:
        print("You are using a virtual environment without access to system packages.")
        print("")
        print("Run the following command to fix:")
        pyvenv_cfg = os.path.join(venv_path, 'pyvenv.cfg')
        print(f"  sed -i 's/include-system-site-packages = false/include-system-site-packages = true/' {pyvenv_cfg} && deactivate && source {venv_path}/bin/activate")
    else:
        print("System python3-gi is not installed.")
        print("")
        print("Run the project installation script:")
        print("  ./install.sh")
    
    print("")
    sys.exit(1)

gi.require_version('Gst', '1.0')
from gi.repository import Gst, GLib

# Initialize GStreamer
Gst.init(None)

# ============================================================================
# Test Statistics (same as C++ version)
# ============================================================================

STATS = {
    'frames_processed': 0,
    'frames_with_metadata': 0,
    'objects_created': 0,
    'objects_read': 0,
    'frame_user_metas_added': 0,
    'frame_user_metas_read': 0,
    'object_user_metas_added': 0,
    'object_user_metas_read': 0,
}

# Metadata type IDs (same as C++ version)
FRAME_ANALYTICS_TYPE = 1001
OBJECT_DETECTION_TYPE = 2001

# ============================================================================
# Test Data Classes (equivalent to C++ structs)
# ============================================================================

class FrameAnalyticsData:
    """
    Frame-level analytics data
    Equivalent to C++ FrameAnalyticsData struct
    """
    def __init__(self, frame_id):
        self.frame_id = frame_id
        self.scene_type = random.choice(["indoor", "outdoor", "street", "highway"])
        self.avg_brightness = random.uniform(50.0, 200.0)
        self.motion_score = random.uniform(0.0, 1.0)
        self.num_objects_detected = 0
    
    def __repr__(self):
        return (f"FrameAnalyticsData(frame_id={self.frame_id}, "
                f"scene={self.scene_type}, "
                f"brightness={self.avg_brightness:.2f}, "
                f"motion={self.motion_score:.2f})")


class ObjectDetectionData:
    """
    Object-level detection data
    Equivalent to C++ ObjectDetectionData struct
    """
    def __init__(self, obj_id, track_id):
        self.object_id = obj_id
        self.tracking_id = track_id
        self.object_type = random.choice(["person", "car", "bike", "truck", "bus"])
        self.detection_confidence = 0.5 + random.random() * 0.5
        self.frame_count = 1
    
    def __repr__(self):
        return (f"ObjectDetectionData(id={self.object_id}, "
                f"track_id={self.tracking_id}, "
                f"type={self.object_type}, "
                f"conf={self.detection_confidence:.2f})")


# ============================================================================
# Probe Callbacks
# ============================================================================

def add_metadata_probe(pad, info, user_data):
    """
    Probe 1: Create metadata and add user metadata
    Equivalent to C++ add_metadata_probe()
    
    This probe:
    1. Creates DXFrameMeta for each frame
    2. Adds frame-level user metadata
    3. Creates multiple DXObjectMeta objects
    4. Adds object-level user metadata to each object
    5. Adds objects to the frame
    """
    with pydxs.writable_buffer(hash(info)) as frame_meta:
        if not frame_meta:
            return Gst.PadProbeReturn.OK
        
        STATS['frames_processed'] += 1
        frame_id = STATS['frames_processed']
        
        # Add frame-level user metadata
        frame_data = FrameAnalyticsData(frame_id)
        frame_meta.dx_add_user_meta_to_frame(frame_data, FRAME_ANALYTICS_TYPE)
        STATS['frame_user_metas_added'] += 1
        
        # Create and add multiple objects (2-5 objects per frame, like C++ version)
        num_objects = random.randint(2, 5)
        for i in range(num_objects):
            # Acquire object meta from pool
            obj_meta = pydxs.dx_acquire_obj_meta_from_pool()
            
            # Set object properties
            obj_meta.label = i % 3  # 0, 1, or 2
            obj_meta.confidence = 0.5 + random.random() * 0.5
            
            # Set bounding box [x, y, width, height]
            obj_meta.box = [
                random.uniform(0, 800),
                random.uniform(0, 600),
                random.uniform(50, 200),
                random.uniform(50, 200)
            ]
            
            # Add object-level user metadata
            obj_data = ObjectDetectionData(i, 1000 + STATS['objects_created'])
            obj_meta.dx_add_user_meta_to_obj(obj_data, OBJECT_DETECTION_TYPE)
            STATS['object_user_metas_added'] += 1
            
            # Add object to frame
            pydxs.dx_add_obj_meta_to_frame(frame_meta, obj_meta)
            STATS['objects_created'] += 1
        
        STATS['frames_with_metadata'] += 1
        
        # Print progress every 10 frames (like C++ version)
        if frame_id % 10 == 0:
            print(f"[CREATE] Frame {frame_id}: Added {num_objects} objects")
    
    return Gst.PadProbeReturn.OK


def read_metadata_probe(pad, info, user_data):
    """
    Probe 2: Read and validate metadata
    Equivalent to C++ read_metadata_probe()
    
    This probe:
    1. Reads DXFrameMeta from buffer
    2. Reads frame-level user metadata
    3. Iterates through all objects in the frame
    4. Reads object-level user metadata from each object
    5. Validates data consistency
    """
    gst_buffer = info.get_buffer()
    if not gst_buffer:
        return Gst.PadProbeReturn.OK
    
    # Get frame metadata
    frame_meta = pydxs.dx_get_frame_meta(hash(gst_buffer))
    if not frame_meta:
        return Gst.PadProbeReturn.OK
    
    # Read frame-level user metadata
    frame_user_metas = frame_meta.dx_get_frame_user_metas()
    STATS['frame_user_metas_read'] += len(frame_user_metas)
    
    # Validate frame user metadata
    for user_meta in frame_user_metas:
        if user_meta.type == FRAME_ANALYTICS_TYPE:
            data = user_meta.get_data()
            # Data should be FrameAnalyticsData instance
            if not isinstance(data, FrameAnalyticsData):
                print(f"WARNING: Invalid frame data type: {type(data)}")
    
    # Read objects and their user metadata
    obj_count = 0
    for obj_meta in frame_meta:
        obj_count += 1
        STATS['objects_read'] += 1
        
        # Read object-level user metadata
        obj_user_metas = obj_meta.dx_get_object_user_metas()
        STATS['object_user_metas_read'] += len(obj_user_metas)
        
        # Validate object user metadata
        for user_meta in obj_user_metas:
            if user_meta.type == OBJECT_DETECTION_TYPE:
                data = user_meta.get_data()
                # Data should be ObjectDetectionData instance
                if not isinstance(data, ObjectDetectionData):
                    print(f"WARNING: Invalid object data type: {type(data)}")
    
    # Print progress every 10 frames (like C++ version)
    if STATS['frames_with_metadata'] % 10 == 0:
        print(f"[READ] Frame {STATS['frames_with_metadata']}: Read {obj_count} objects")
    
    return Gst.PadProbeReturn.OK


# ============================================================================
# Main Test Function
# ============================================================================

def run_test():
    """
    Run the user metadata test with 50 frames
    
    Returns:
        True if test passed, False otherwise
    """
    NUM_FRAMES = 50  # Fixed number of frames for testing
    
    print("=" * 62)
    print("DX-Stream User Metadata Test (Python)")
    print("=" * 62)
    print(f"Test mode: videotestsrc (no external video file needed)")
    print(f"Frames to test: {NUM_FRAMES}")
    print("=" * 62)
    
    # Create pipeline using GStreamer API
    pipeline = Gst.Pipeline.new("test-usermeta-pipeline")
    
    # Create elements (without identity)
    videotestsrc = Gst.ElementFactory.make("videotestsrc", "source")
    capsfilter = Gst.ElementFactory.make("capsfilter", "caps")
    convert1 = Gst.ElementFactory.make("videoconvert", "convert1")
    convert2 = Gst.ElementFactory.make("videoconvert", "convert2")
    fakesink = Gst.ElementFactory.make("fakesink", "sink")
    
    if not all([videotestsrc, capsfilter, convert1, convert2, fakesink]):
        print("ERROR: Failed to create pipeline elements")
        return False
    
    # Set element properties
    videotestsrc.set_property("num-buffers", NUM_FRAMES)
    
    # Set caps for video format
    caps = Gst.Caps.from_string("video/x-raw,width=1920,height=1080,framerate=30/1")
    capsfilter.set_property("caps", caps)
    
    fakesink.set_property("sync", False)
    
    # Add elements to pipeline
    pipeline.add(videotestsrc)
    pipeline.add(capsfilter)
    pipeline.add(convert1)
    pipeline.add(convert2)
    pipeline.add(fakesink)
    
    # Link elements
    if not videotestsrc.link(capsfilter):
        print("ERROR: Failed to link videotestsrc -> capsfilter")
        return False
    
    if not capsfilter.link(convert1):
        print("ERROR: Failed to link capsfilter -> convert1")
        return False
    
    if not convert1.link(convert2):
        print("ERROR: Failed to link convert1 -> convert2")
        return False
    
    if not convert2.link(fakesink):
        print("ERROR: Failed to link convert2 -> fakesink")
        return False
    
    # Setup message handling
    loop = GLib.MainLoop()
    test_result = {'success': True}
    
    def on_message(bus, message):
        mtype = message.type
        if mtype == Gst.MessageType.EOS:
            print("\nEnd of stream")
            loop.quit()
        elif mtype == Gst.MessageType.ERROR:
            err, debug = message.parse_error()
            print(f"\nERROR: {err}")
            if debug:
                print(f"DEBUG: {debug}")
            test_result['success'] = False
            loop.quit()
        return True
    
    bus = pipeline.get_bus()
    bus.add_signal_watch()
    bus.connect("message", on_message)
    
    # Attach probes (same as C++ version and usermeta_app.py)
    convert1_src = convert1.get_static_pad("src")
    convert2_src = convert2.get_static_pad("src")
    
    if not convert1_src or not convert2_src:
        print("ERROR: Failed to get element pads")
        return False
    
    convert1_src.add_probe(Gst.PadProbeType.BUFFER, add_metadata_probe, None)
    convert2_src.add_probe(Gst.PadProbeType.BUFFER, read_metadata_probe, None)
    
    # Start pipeline
    print("\nStarting pipeline...\n")
    ret = pipeline.set_state(Gst.State.PLAYING)
    if ret == Gst.StateChangeReturn.FAILURE:
        print("ERROR: Unable to set pipeline to PLAYING")
        return False
    
    # Run main loop
    try:
        loop.run()
    except KeyboardInterrupt:
        print("\nInterrupted by user")
        test_result['success'] = False
    
    # Cleanup
    pipeline.set_state(Gst.State.NULL)
    
    return test_result['success']


def print_test_results():
    """
    Print test statistics and validation results
    Same format as C++ version
    """
    print("\n" + "=" * 62)
    print("TEST RESULTS")
    print("=" * 62)
    
    print(f"\nFrames:")
    print(f"  Processed:        {STATS['frames_processed']}")
    print(f"  With metadata:    {STATS['frames_with_metadata']}")
    
    print(f"\nObjects:")
    print(f"  Created:          {STATS['objects_created']}")
    print(f"  Read:             {STATS['objects_read']}")
    
    print(f"\nUser Metadata:")
    print(f"  Frame added:      {STATS['frame_user_metas_added']}")
    print(f"  Frame read:       {STATS['frame_user_metas_read']}")
    print(f"  Object added:     {STATS['object_user_metas_added']}")
    print(f"  Object read:      {STATS['object_user_metas_read']}")
    
    # Validation
    print(f"\nValidation:")
    all_passed = True
    
    # Check 1: Frames processed
    if STATS['frames_processed'] == 0:
        print("  FAIL: No frames processed")
        all_passed = False
    else:
        print("  PASS: Frames processed")
    
    # Check 2: Frame metadata consistency
    if STATS['frame_user_metas_added'] != STATS['frame_user_metas_read']:
        print(f"  FAIL: Frame metadata mismatch "
              f"(added: {STATS['frame_user_metas_added']}, "
              f"read: {STATS['frame_user_metas_read']})")
        all_passed = False
    else:
        print("  PASS: Frame metadata consistent")
    
    # Check 3: Object metadata consistency
    if STATS['object_user_metas_added'] != STATS['object_user_metas_read']:
        print(f"  FAIL: Object metadata mismatch "
              f"(added: {STATS['object_user_metas_added']}, "
              f"read: {STATS['object_user_metas_read']})")
        all_passed = False
    else:
        print("  PASS: Object metadata consistent")
    
    # Check 4: Object count consistency
    if STATS['objects_created'] != STATS['objects_read']:
        print(f"  FAIL: Object count mismatch "
              f"(created: {STATS['objects_created']}, "
              f"read: {STATS['objects_read']})")
        all_passed = False
    else:
        print("  PASS: Object count consistent")
    
    print("=" * 62)
    if all_passed:
        print("TEST PASSED")
    else:
        print("TEST FAILED")
    print("=" * 62)
    
    return all_passed


def main():
    """Main entry point"""
    # Run test with fixed 50 frames
    test_passed = run_test()
    
    # Print results
    validation_passed = print_test_results()
    
    # Return exit code
    if test_passed and validation_passed:
        return 0
    else:
        return 1


if __name__ == "__main__":
    sys.exit(main())
