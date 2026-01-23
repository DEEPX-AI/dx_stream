#!/usr/bin/env python3
"""
DX-Stream User Metadata Pipeline Example (Python)

This example demonstrates how to:
1. Create DXFrameMeta and DXObjectMeta from scratch (like C++ version)
2. Add custom user metadata to frames and objects
3. Read and display user metadata from the pipeline
4. Use multiple types of custom metadata (frame-level and object-level)

This is the Python equivalent of usermeta_app.cpp, showing that
Python can also create and manage metadata, not just read existing ones.

Usage:
    python usermeta_app.py <video_file_path>
    
Example:
    python usermeta_app.py ../../samples/videos/boat.mp4
"""

import sys
import os
import random
import time
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
# Custom User Metadata Classes
# ============================================================================

class FrameAnalyticsData:
    """Frame-level analytics metadata"""
    def __init__(self):
        self.scene_type = ""           # "indoor", "outdoor", "street", etc.
        self.brightness_level = 0.0    # 0.0 - 1.0
        self.motion_level = 0.0        # 0.0 - 1.0
        self.timestamp = 0
        self.detected_objects = 0
    
    def __repr__(self):
        return (f"FrameAnalytics(scene='{self.scene_type}', "
                f"brightness={self.brightness_level:.2f}, "
                f"motion={self.motion_level:.2f}, "
                f"objects={self.detected_objects})")


class ObjectDetectionData:
    """Object-level detection metadata"""
    def __init__(self):
        self.object_type = ""          # "person", "car", "bike", etc.
        self.detection_confidence = 0.0
        self.quality_score = 0.0       # object quality assessment
        self.tracking_id = 0
        self.bbox = [0.0, 0.0, 0.0, 0.0]  # x, y, w, h
    
    def __repr__(self):
        return (f"ObjectDetection(type='{self.object_type}', "
                f"conf={self.detection_confidence:.2f}, "
                f"quality={self.quality_score:.2f}, "
                f"track_id={self.tracking_id})")


class QualityAssessmentData:
    """Frame quality assessment metadata"""
    def __init__(self):
        self.overall_quality = 0.0     # 0.0 - 1.0
        self.sharpness = 0.0
        self.exposure = 0.0
        self.quality_grade = ""        # "excellent", "good", "fair", "poor"
    
    def __repr__(self):
        return (f"QualityAssessment(grade='{self.quality_grade}', "
                f"score={self.overall_quality:.2f}, "
                f"sharpness={self.sharpness:.2f})")


# Custom metadata type IDs
FRAME_ANALYTICS_TYPE = 1001
OBJECT_DETECTION_TYPE = 2001
QUALITY_ASSESSMENT_TYPE = 1002

# ============================================================================
# Helper Functions
# ============================================================================

SCENE_TYPES = ["indoor", "outdoor", "street", "office", "home", "park"]
OBJECT_TYPES = ["person", "car", "bike", "dog", "cat", "truck", "bus"]

def get_random_scene_type():
    """Get random scene type for simulation"""
    return random.choice(SCENE_TYPES)  # noqa: S311

def get_random_object_type():
    """Get random object type for simulation"""
    return random.choice(OBJECT_TYPES)  # noqa: S311

def get_quality_grade(score):
    """Convert quality score to grade string"""
    if score >= 0.8:
        return "excellent"
    elif score >= 0.6:
        return "good"
    elif score >= 0.4:
        return "fair"
    else:
        return "poor"

# ============================================================================
# Global State
# ============================================================================

frame_count = 0
tracking_id_counter = 1000

# ============================================================================
# Probe Callbacks
# ============================================================================

def add_metadata_probe(pad, info, user_data):
    """
    PROBE 1: Create metadata from scratch and add user metadata
    
    This probe demonstrates the FULL metadata lifecycle (like C++ version):
    1. Create DXFrameMeta from scratch
    2. Create DXObjectMeta and add to frame
    3. Add custom user metadata to both frame and objects
    """
    global frame_count, tracking_id_counter
    
    # Use the new Context Manager for safe writable buffer access
    # This handles the writability check and metadata creation automatically
    with pydxs.writable_buffer(hash(info)) as frame_meta:
        if not frame_meta:
            return Gst.PadProbeReturn.OK
        
        frame_count += 1
        
        # Only print for first few frames to avoid spam
        if frame_count <= 5:
            print(f"\n[PROBE 1] Adding metadata to frame #{frame_count}")
            print("  > Created new DXFrameMeta")

        # === Add Frame Analytics Data ===
        frame_analytics = FrameAnalyticsData()
        frame_analytics.scene_type = get_random_scene_type()
        frame_analytics.brightness_level = random.random()  # noqa: S311
        frame_analytics.motion_level = random.random()  # noqa: S311
        frame_analytics.timestamp = int(time.time() * 1000000)  # microseconds
        
        # Simulate detected objects count (1-5 objects)
        frame_analytics.detected_objects = random.randint(1, 5)  # noqa: S311
        
        frame_meta.dx_add_user_meta_to_frame(frame_analytics, FRAME_ANALYTICS_TYPE)
        
        if frame_count <= 5:
            print(f"  > Added frame analytics: scene='{frame_analytics.scene_type}', "
                  f"brightness={frame_analytics.brightness_level:.2f}, "
                  f"motion={frame_analytics.motion_level:.2f}")
        
        # === Add Quality Assessment Data ===
        quality_data = QualityAssessmentData()
        quality_data.overall_quality = random.random()  # noqa: S311
        quality_data.sharpness = random.random()  # noqa: S311
        quality_data.exposure = random.random()  # noqa: S311
        quality_data.quality_grade = get_quality_grade(quality_data.overall_quality)
        
        frame_meta.dx_add_user_meta_to_frame(quality_data, QUALITY_ASSESSMENT_TYPE)
        
        if frame_count <= 5:
            print(f"  > Added quality assessment: grade='{quality_data.quality_grade}', "
                  f"score={quality_data.overall_quality:.2f}")
        
        # === CREATE and ADD Object Metadata ===
        for i in range(frame_analytics.detected_objects):
            # Acquire object metadata from pool (like C++)
            obj_meta = pydxs.dx_acquire_obj_meta_from_pool()
            
            # Set object properties
            obj_meta.label = i % 3  # Simulate different classes (0, 1, 2)
            obj_meta.confidence = 0.5 + random.random() * 0.5  # 0.5-1.0  # noqa: S311
            
            # Set bounding box (simulate random positions)
            obj_meta.box = [
                random.uniform(0.0, 800.0),   # x  # noqa: S311
                random.uniform(0.0, 600.0),   # y  # noqa: S311
                random.uniform(50.0, 250.0),  # width  # noqa: S311
                random.uniform(50.0, 250.0)   # height  # noqa: S311
            ]
            
            # Add object detection user metadata
            obj_detection = ObjectDetectionData()
            obj_detection.object_type = get_random_object_type()
            obj_detection.detection_confidence = 0.5 + random.random() * 0.5  # 0.5-1.0  # noqa: S311
            obj_detection.quality_score = random.random()  # noqa: S311
            obj_detection.tracking_id = tracking_id_counter
            tracking_id_counter += 1
            obj_detection.bbox = obj_meta.box.copy()
            
            obj_meta.dx_add_user_meta_to_obj(obj_detection, OBJECT_DETECTION_TYPE)
            
            if frame_count <= 5:
                print(f"  > Added object {i+1}: type='{obj_detection.object_type}', "
                      f"confidence={obj_detection.detection_confidence:.2f}, "
                      f"bbox=({obj_detection.bbox[0]:.1f},{obj_detection.bbox[1]:.1f},"
                      f"{obj_detection.bbox[2]:.1f},{obj_detection.bbox[3]:.1f})")
            
            # Add object to frame (like C++ dx_add_obj_meta_to_frame)
            pydxs.dx_add_obj_meta_to_frame(frame_meta, obj_meta)
        
        if frame_count <= 5:
            print(f"  > Frame #{frame_count}: Added {frame_analytics.detected_objects} objects with metadata")
    
    return Gst.PadProbeReturn.OK


def read_metadata_probe(pad, info, user_data):
    """
    PROBE 2: Read and display user metadata
    """
    gst_buffer = info.get_buffer()
    if not gst_buffer:
        return Gst.PadProbeReturn.OK
    
    buffer_addr = hash(gst_buffer)
    display_count = user_data['display_count']
    display_count += 1
    user_data['display_count'] = display_count
    
    # Get frame metadata
    frame_meta = pydxs.dx_get_frame_meta(buffer_addr)
    if not frame_meta:
        if display_count <= 5:
             print(f"[PROBE 2] No metadata found for frame #{display_count}")
        return Gst.PadProbeReturn.OK
    
    # Only print for first few frames
    if display_count > 5:
        return Gst.PadProbeReturn.OK
    
    print(f"\n[PROBE 2] Reading metadata from frame #{display_count}")
    
    # === Read Frame User Metadata ===
    frame_user_metas = frame_meta.dx_get_frame_user_metas()
    print(f"  > Found {len(frame_user_metas)} frame user metadata entries")
    
    for user_meta in frame_user_metas:
        data = user_meta.get_data()
        
        if user_meta.type == FRAME_ANALYTICS_TYPE:
            if isinstance(data, FrameAnalyticsData):
                print(f"    * Frame Analytics: scene='{data.scene_type}', "
                      f"brightness={data.brightness_level:.2f}, "
                      f"motion={data.motion_level:.2f}, "
                      f"objects={data.detected_objects}")
        
        elif user_meta.type == QUALITY_ASSESSMENT_TYPE:
            if isinstance(data, QualityAssessmentData):
                print(f"    * Quality Assessment: grade='{data.quality_grade}', "
                      f"score={data.overall_quality:.2f}, "
                      f"sharpness={data.sharpness:.2f}")
    
    # === Read Object Metadata ===
    # Use the new iterator support for cleaner code
    objects = list(frame_meta)  # Or just iterate directly: for obj_meta in frame_meta:
    print(f"  > Found {len(objects)} objects")
    
    for i, obj_meta in enumerate(frame_meta):
        print(f"    * Object #{i+1}: class={obj_meta.label}, "
              f"bbox=({obj_meta.box[0]:.1f},{obj_meta.box[1]:.1f},"
              f"{obj_meta.box[2]:.1f},{obj_meta.box[3]:.1f})")
        
        # Read object user metadata
        obj_user_metas = obj_meta.dx_get_object_user_metas()
        for obj_user_meta in obj_user_metas:
            if obj_user_meta.type == OBJECT_DETECTION_TYPE:
                detection = obj_user_meta.get_data()
                if isinstance(detection, ObjectDetectionData):
                    print(f"      > Detection: type='{detection.object_type}', "
                          f"confidence={detection.detection_confidence:.2f}, "
                          f"quality={detection.quality_score:.2f}")
    
    print(f"  > Total objects processed: {len(objects)}")
    
    return Gst.PadProbeReturn.OK


# ============================================================================
# Message Handling
# ============================================================================

def on_message(bus, message, loop):
    """Handle GStreamer bus messages"""
    mtype = message.type
    
    if mtype == Gst.MessageType.EOS:
        print("\n" + "="*60)
        print("End of stream")
        print("="*60)
        loop.quit()
    
    elif mtype == Gst.MessageType.ERROR:
        err, debug = message.parse_error()
        print(f"\nERROR: {err}")
        print(f"DEBUG: {debug}")
        loop.quit()
    
    elif mtype == Gst.MessageType.WARNING:
        err, debug = message.parse_warning()
        print(f"\nWARNING: {err}")
    
    return True


# ============================================================================
# Main Function
# ============================================================================

def main():
    """Main function to create and run the pipeline"""
    
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <video_file_path>")
        print(f"Example: {sys.argv[0]} /path/to/video.mp4")
        return -1
    
    video_path = os.path.abspath(sys.argv[1])
    
    if not os.path.exists(video_path):
        print(f"ERROR: Video file not found: {video_path}")
        return -1
    
    print("\n" + "="*60)
    print("🚀 DX-Stream User Meta Pipeline Example (Python)")
    print("="*60)
    print("📹 Input file:", video_path)
    print("="*60)
    print("\nThis example demonstrates the SAME scenario as usermeta_app.cpp:")
    print("  - Create DXFrameMeta from scratch")
    print("  - Create DXObjectMeta and add to frame")
    print("  - Add custom user metadata")
    print("  - Read and display all metadata")
    print("="*60 + "\n")
    
    # Create pipeline using GStreamer API
    pipeline = Gst.Pipeline.new("dx-stream-usermeta-pipeline")
    
    # Create elements (without identity)
    filesrc = Gst.ElementFactory.make("filesrc", "source")
    decodebin = Gst.ElementFactory.make("decodebin", "decoder")
    convert1 = Gst.ElementFactory.make("videoconvert", "convert1")
    convert2 = Gst.ElementFactory.make("videoconvert", "convert2")
    fakesink = Gst.ElementFactory.make("fakesink", "sink")
    
    if not all([filesrc, decodebin, convert1, convert2, fakesink]):
        print("ERROR: Failed to create pipeline elements")
        return -1
    
    # Set element properties
    filesrc.set_property("location", video_path)
    fakesink.set_property("sync", False)
    
    # Add elements to pipeline
    pipeline.add(filesrc)
    pipeline.add(decodebin)
    pipeline.add(convert1)
    pipeline.add(convert2)
    pipeline.add(fakesink)
    
    # Link elements (decodebin needs dynamic pad handling)
    if not filesrc.link(decodebin):
        print("ERROR: Failed to link filesrc -> decodebin")
        return -1
    
    if not convert1.link(convert2):
        print("ERROR: Failed to link convert1 -> convert2")
        return -1
    
    if not convert2.link(fakesink):
        print("ERROR: Failed to link convert2 -> fakesink")
        return -1
    
    # Handle decodebin dynamic pad
    def on_pad_added(element, pad):
        sink_pad = convert1.get_static_pad("sink")
        if not sink_pad.is_linked():
            pad.link(sink_pad)
    
    decodebin.connect("pad-added", on_pad_added)
    
    print(f"Pipeline created with elements:")
    print(f"  filesrc -> decodebin -> convert1 -> convert2 -> fakesink\n")
    
    # Create main loop first (for error handling)
    loop = GLib.MainLoop()
    
    # Connect bus signals BEFORE setting state
    bus = pipeline.get_bus()
    bus.add_signal_watch()
    bus.connect("message", on_message, loop)
    
    # Add probe 1: Create and add metadata (on convert1 src pad)
    convert1_src = convert1.get_static_pad("src")
    if not convert1_src:
        print("ERROR: Failed to get convert1 src pad")
        return -1
    
    convert1_src.add_probe(Gst.PadProbeType.BUFFER, add_metadata_probe, None)
    print("✓ Probe 1 attached to convert1 src pad (create & add metadata)")
    
    # Add probe 2: Read metadata (on convert2 src pad)
    convert2_src = convert2.get_static_pad("src")
    if not convert2_src:
        print("ERROR: Failed to get convert2 src pad")
        return -1
    
    probe_data = {'display_count': 0}
    convert2_src.add_probe(Gst.PadProbeType.BUFFER, read_metadata_probe, probe_data)
    print("✓ Probe 2 attached to convert2 src pad (read metadata)")
    print()
    
    # Start pipeline
    print("🎬 Starting pipeline...")
    ret = pipeline.set_state(Gst.State.PLAYING)
    if ret == Gst.StateChangeReturn.FAILURE:
        print("ERROR: Unable to set pipeline to PLAYING state")
        print("Waiting for error message...")
        GLib.timeout_add_seconds(2, lambda: loop.quit())
        loop.run()
        pipeline.set_state(Gst.State.NULL)
        return -1
    
    # Run main loop
    try:
        loop.run()
    except KeyboardInterrupt:
        print("\n\nInterrupted by user")
    
    # Cleanup
    print("\nCleaning up...")
    pipeline.set_state(Gst.State.NULL)
    
    print("User Meta Pipeline Example completed!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
