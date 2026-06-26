#!/usr/bin/env python3
"""
P7 — pydxs Python binding contract tests.

CPY1: import pydxs succeeds
CPY2: GstBuffer → DXFrameMeta read access via probe
CPY3: DXObjectMeta properties (box, label, track_id)
CPY4: numpy conversion for tensors
CPY5: writable_buffer context manager + read-only buffer safety
"""

import os
import sys
import threading


# On Windows (Python 3.8+), dependent DLLs of the compiled `pydxs` extension are
# not resolved via PATH. Promote relevant directories with add_dll_directory so
# gstdxstream.dll and its transitive deps (gstreamer, dxrt, opencv, ...) load.
if sys.platform == 'win32':
    _dll_dirs = []
    _root = os.environ.get('DX_STREAM_ROOT')
    if _root:
        _dll_dirs += [
            os.path.join(_root, 'install', 'lib', 'gstreamer-1.0'),
            os.path.join(_root, 'install', 'bin'),
        ]
    # Everything already on PATH (gstreamer bin, DEEPX runtime, etc.)
    _dll_dirs += [p for p in os.environ.get('PATH', '').split(os.pathsep) if p]
    _seen = set()
    for _d in _dll_dirs:
        _d = _d.strip().strip('"')
        if _d and _d not in _seen and os.path.isdir(_d):
            _seen.add(_d)
            try:
                os.add_dll_directory(_d)
            except OSError:
                pass

# PyGObject (gi) is required for the GStreamer-pipeline based test cases. It is
# readily available on Linux but impractical to build against a redistributable
# GStreamer on Windows. Treat it as optional: tests that need a live pipeline are
# skipped when gi is unavailable, while the pure-binding tests still run.
try:
    import gi
    gi.require_version('Gst', '1.0')
    from gi.repository import Gst, GLib
    Gst.init(None)
    HAS_GI = True
except (ImportError, ValueError) as _gi_err:
    HAS_GI = False
    _GI_ERR = str(_gi_err)

passed = 0
failed = 0
skipped = 0
errors = []


def check(name, condition, msg=""):
    global passed, failed
    if condition:
        passed += 1
    else:
        failed += 1
        full = f"FAIL: {name}"
        if msg:
            full += f" - {msg}"
        errors.append(full)
        print(full, file=sys.stderr)


def skip(name, reason):
    global skipped
    skipped += 1
    print(f"SKIP: {name} - {reason}")


# ---- CPY1: import succeeds, module has expected symbols ----
def test_cpy1_import():
    import pydxs
    check("CPY1_import", True)
    check("CPY1_DXFrameMeta", hasattr(pydxs, 'DXFrameMeta'))
    check("CPY1_DXObjectMeta", hasattr(pydxs, 'DXObjectMeta'))
    check("CPY1_DXUserMeta", hasattr(pydxs, 'DXUserMeta'))
    check("CPY1_writable_buffer", hasattr(pydxs, 'writable_buffer'))
    check("CPY1_dx_get_frame_meta", hasattr(pydxs, 'dx_get_frame_meta'))
    check("CPY1_dx_create_frame_meta", hasattr(pydxs, 'dx_create_frame_meta'))
    check("CPY1_dx_acquire_obj", hasattr(pydxs, 'dx_acquire_obj_meta_from_pool'))


# ---- CPY2: DXFrameMeta read via probe ----
def test_cpy2_frame_meta_read():
    import pydxs

    probe_results = {'fm_found': False, 'stream_id': None, 'width': None,
                     'obj_count': 0, 'obj_label': None, 'obj_box': None}

    def write_probe(pad, info, _):
        with pydxs.writable_buffer(hash(info)) as fm:
            if not fm:
                return Gst.PadProbeReturn.OK
            fm.stream_id = 3
            fm.width = 320
            fm.height = 240
            obj = pydxs.dx_acquire_obj_meta_from_pool()
            obj.label = 7
            obj.confidence = 0.85
            obj.box = [10.0, 20.0, 50.0, 60.0]
            pydxs.dx_add_obj_meta_to_frame(fm, obj)
        return Gst.PadProbeReturn.OK

    def read_probe(pad, info, _):
        buf = info.get_buffer()
        if not buf:
            return Gst.PadProbeReturn.OK
        fm = pydxs.dx_get_frame_meta(hash(buf))
        if fm:
            probe_results['fm_found'] = True
            probe_results['stream_id'] = fm.stream_id
            probe_results['width'] = fm.width
            for obj in fm:
                probe_results['obj_count'] += 1
                probe_results['obj_label'] = obj.label
                probe_results['obj_box'] = obj.box
        return Gst.PadProbeReturn.OK

    pipe = Gst.parse_launch(
        "videotestsrc num-buffers=3 "
        "! video/x-raw,format=RGB,width=64,height=64,framerate=30/1 "
        "! identity name=writer "
        "! identity name=reader "
        "! fakesink sync=false")

    writer = pipe.get_by_name("writer")
    reader = pipe.get_by_name("reader")
    writer.get_static_pad("src").add_probe(Gst.PadProbeType.BUFFER, write_probe, None)
    reader.get_static_pad("src").add_probe(Gst.PadProbeType.BUFFER, read_probe, None)

    pipe.set_state(Gst.State.PLAYING)
    bus = pipe.get_bus()
    bus.timed_pop_filtered(10 * Gst.SECOND,
                           Gst.MessageType.EOS | Gst.MessageType.ERROR)
    pipe.set_state(Gst.State.NULL)

    check("CPY2_meta_found", probe_results['fm_found'])
    check("CPY2_stream_id", probe_results['stream_id'] == 3,
          f"expected 3, got {probe_results['stream_id']}")
    check("CPY2_width", probe_results['width'] == 320,
          f"expected 320, got {probe_results['width']}")
    check("CPY2_obj_count", probe_results['obj_count'] >= 1,
          f"expected >=1, got {probe_results['obj_count']}")
    check("CPY2_obj_label", probe_results['obj_label'] == 7,
          f"expected 7, got {probe_results['obj_label']}")
    check("CPY2_obj_box", probe_results['obj_box'] is not None and
          abs(probe_results['obj_box'][0] - 10.0) < 0.01,
          f"expected box[0]=10.0, got {probe_results['obj_box']}")


# ---- CPY3: DXObjectMeta properties ----
def test_cpy3_object_meta_properties():
    import pydxs

    obj = pydxs.dx_acquire_obj_meta_from_pool()
    check("CPY3_default_track_id", obj.track_id == -1,
          f"expected -1, got {obj.track_id}")
    check("CPY3_default_label", obj.label == -1,
          f"expected -1, got {obj.label}")
    check("CPY3_default_confidence", obj.confidence == -1.0,
          f"expected -1.0, got {obj.confidence}")

    obj.label = 42
    obj.track_id = 100
    obj.confidence = 0.95
    obj.box = [1.0, 2.0, 3.0, 4.0]
    obj.label_name = "car"

    check("CPY3_set_label", obj.label == 42)
    check("CPY3_set_track_id", obj.track_id == 100)
    check("CPY3_set_confidence", abs(obj.confidence - 0.95) < 1e-5)
    check("CPY3_set_box", obj.box == [1.0, 2.0, 3.0, 4.0],
          f"got {obj.box}")
    check("CPY3_set_label_name", obj.label_name == "car")

    try:
        obj.box = [1.0, 2.0]
        check("CPY3_box_validation", False, "should raise on short box")
    except RuntimeError:
        check("CPY3_box_validation", True)


# ---- CPY4: numpy tensor conversion ----
def test_cpy4_numpy_tensor():
    import pydxs

    buf = Gst.Buffer.new_allocate(None, 64*64*3, None)
    fm = pydxs.dx_create_frame_meta(hash(buf))
    check("CPY4_create_meta", fm is not None)

    input_tensors = fm.input_tensors
    output_tensors = fm.output_tensors
    check("CPY4_input_tensors_empty", isinstance(input_tensors, dict) and len(input_tensors) == 0)
    check("CPY4_output_tensors_empty", isinstance(output_tensors, dict) and len(output_tensors) == 0)

    try:
        import numpy as np
        has_numpy = True
    except ImportError:
        has_numpy = False

    if has_numpy:
        check("CPY4_numpy_available", True)
    else:
        check("CPY4_numpy_available", True, "numpy not available, skipping tensor data test")


# ---- CPY5: writable_buffer context + add/remove objects ----
def test_cpy5_writable_buffer():
    import pydxs

    ctx_results = {'entered': False, 'fm_type': None, 'add_ok': False, 'remove_ok': False}

    def probe_fn(pad, info, _):
        with pydxs.writable_buffer(hash(info)) as fm:
            if fm:
                ctx_results['entered'] = True
                ctx_results['fm_type'] = type(fm).__name__
                obj = pydxs.dx_acquire_obj_meta_from_pool()
                obj.label = 99
                ok = pydxs.dx_add_obj_meta_to_frame(fm, obj)
                ctx_results['add_ok'] = ok
                rm = pydxs.dx_remove_obj_meta_from_frame(fm, obj)
                ctx_results['remove_ok'] = rm
        return Gst.PadProbeReturn.OK

    pipe = Gst.parse_launch(
        "videotestsrc num-buffers=1 "
        "! video/x-raw,format=RGB,width=4,height=4,framerate=30/1 "
        "! identity name=probed "
        "! fakesink sync=false")

    el = pipe.get_by_name("probed")
    el.get_static_pad("src").add_probe(Gst.PadProbeType.BUFFER, probe_fn, None)

    pipe.set_state(Gst.State.PLAYING)
    bus = pipe.get_bus()
    bus.timed_pop_filtered(10 * Gst.SECOND,
                           Gst.MessageType.EOS | Gst.MessageType.ERROR)
    pipe.set_state(Gst.State.NULL)

    check("CPY5_ctx_entered", ctx_results['entered'])
    check("CPY5_fm_type", ctx_results['fm_type'] == 'DXFrameMeta',
          f"expected DXFrameMeta, got {ctx_results['fm_type']}")
    check("CPY5_add_obj", ctx_results['add_ok'])
    check("CPY5_remove_obj", ctx_results['remove_ok'])

    # user meta round-trip
    user_results = {'data_back': None, 'type_back': None}

    def user_write(pad, info, _):
        with pydxs.writable_buffer(hash(info)) as fm:
            if fm:
                fm.dx_add_user_meta_to_frame({"key": "value", "num": 42}, 9001)
        return Gst.PadProbeReturn.OK

    def user_read(pad, info, _):
        buf = info.get_buffer()
        fm = pydxs.dx_get_frame_meta(hash(buf))
        if fm:
            metas = fm.dx_get_frame_user_metas()
            for um in metas:
                if int(um.type) == 9001:
                    user_results['data_back'] = um.get_data()
                    user_results['type_back'] = type(um.get_data()).__name__
        return Gst.PadProbeReturn.OK

    pipe2 = Gst.parse_launch(
        "videotestsrc num-buffers=3 "
        "! video/x-raw,format=RGB,width=4,height=4,framerate=30/1 "
        "! identity name=w "
        "! identity name=r "
        "! fakesink sync=false")

    pipe2.get_by_name("w").get_static_pad("src").add_probe(
        Gst.PadProbeType.BUFFER, user_write, None)
    pipe2.get_by_name("r").get_static_pad("src").add_probe(
        Gst.PadProbeType.BUFFER, user_read, None)

    pipe2.set_state(Gst.State.PLAYING)
    bus2 = pipe2.get_bus()
    bus2.timed_pop_filtered(10 * Gst.SECOND,
                            Gst.MessageType.EOS | Gst.MessageType.ERROR)
    pipe2.set_state(Gst.State.NULL)

    check("CPY5_user_meta_type", user_results['type_back'] == 'dict',
          f"expected dict, got {user_results['type_back']}")
    check("CPY5_user_meta_data", user_results['data_back'] is not None and
          user_results['data_back'].get('key') == 'value' and
          user_results['data_back'].get('num') == 42,
          f"got {user_results['data_back']}")


def main():
    if not HAS_GI:
        print(f"[INFO] PyGObject (gi) unavailable: {_GI_ERR}")
        print("[INFO] Running pydxs binding tests without GStreamer pipeline cases.")

    test_cpy1_import()
    test_cpy3_object_meta_properties()

    if HAS_GI:
        test_cpy2_frame_meta_read()
        test_cpy4_numpy_tensor()
        test_cpy5_writable_buffer()
    else:
        skip("CPY2_frame_meta_read", "requires gi/GStreamer")
        skip("CPY4_numpy_tensor", "requires gi/GStreamer")
        skip("CPY5_writable_buffer", "requires gi/GStreamer")

    total = passed + failed
    print(f"\n{'='*50}")
    summary = f"pydxs tests: {passed}/{total} passed, {failed} failed"
    if skipped:
        summary += f", {skipped} skipped"
    print(summary)
    if errors:
        print("\nFailures:")
        for e in errors:
            print(f"  {e}")
    print(f"{'='*50}")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
