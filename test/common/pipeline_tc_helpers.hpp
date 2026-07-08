// L2 Pipeline common TC helpers
// Include from each topology file and call with its own pipeline string.
// Can be skipped depending on topology characteristics (multi-stream, etc.).

#pragma once

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/video/video.h>
#include "pipeline_helpers.hpp"
#include "meta_helpers.hpp"
#include "npu_env.hpp"

#include <cstring>
#include <functional>
#include <string>

namespace dxtest {

// Repeat PLAYING->NULL N times -- detect resource leaks
inline void test_lifecycle_cycle(const char *desc, int cycles = 5) {
    for (int i = 0; i < cycles; i++) {
        GError *err = nullptr;
        GstElement *pipe = gst_parse_launch(desc, &err);
        fail_unless(pipe != nullptr, "cycle %d: parse_launch failed: %s",
                    i, err ? err->message : "unknown");
        if (err) g_error_free(err);

        GstStateChangeReturn r;
        r = gst_element_set_state(pipe, GST_STATE_PLAYING);
        fail_unless(r != GST_STATE_CHANGE_FAILURE,
                    "cycle %d: PLAYING failed", i);

        g_usleep(100 * 1000);

        r = gst_element_set_state(pipe, GST_STATE_NULL);
        fail_unless(r != GST_STATE_CHANGE_FAILURE,
                    "cycle %d: NULL failed", i);

        gst_object_unref(pipe);
    }
}

// Verify EOS propagates through the entire chain
inline void test_eos_propagation(const char *desc,
                                 GstClockTime timeout = 15 * GST_SECOND) {
    GError *err = nullptr;
    GstElement *pipe = gst_parse_launch(desc, &err);
    fail_unless(pipe != nullptr, "parse_launch failed: %s",
                err ? err->message : "unknown");
    if (err) g_error_free(err);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstMessage *msg = gst_bus_timed_pop_filtered(bus, timeout,
        (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    fail_unless(msg != nullptr, "timeout waiting for EOS or ERROR");

    if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
        GError *gerr = nullptr;
        gst_message_parse_error(msg, &gerr, nullptr);
        fail("unexpected bus ERROR: %s", gerr ? gerr->message : "unknown");
        g_error_free(gerr);
    }
    fail_unless(GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS,
                "expected EOS, got %s",
                gst_message_type_get_name(GST_MESSAGE_TYPE(msg)));
    gst_message_unref(msg);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipe);
}

// Running pipeline with invalid config should produce bus ERROR + recover to NULL
inline void test_error_recovery(const char *desc) {
    GError *err = nullptr;
    GstElement *pipe = gst_parse_launch(desc, &err);
    if (!pipe) {
        if (err) g_error_free(err);
        return;
    }
    if (err) g_error_free(err);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    GstStateChangeReturn r = gst_element_set_state(pipe, GST_STATE_PLAYING);

    if (r == GST_STATE_CHANGE_FAILURE) {
        gst_element_set_state(pipe, GST_STATE_NULL);
        gst_object_unref(bus);
        gst_object_unref(pipe);
        return;
    }

    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    if (msg) {
        fail_unless(GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR,
                    "expected ERROR from bad config pipeline");
        gst_message_unref(msg);
    }

    r = gst_element_set_state(pipe, GST_STATE_NULL);
    fail_unless(r != GST_STATE_CHANGE_FAILURE,
                "NULL recovery after error must succeed");

    gst_object_unref(bus);
    gst_object_unref(pipe);
}

// Verify LATENCY query traverses the pipeline chain and gets a response
inline void test_latency_query(const char *desc) {
    GError *err = nullptr;
    GstElement *pipe = gst_parse_launch(desc, &err);
    fail_unless(pipe != nullptr);
    if (err) g_error_free(err);

    gst_element_set_state(pipe, GST_STATE_PAUSED);
    gst_element_get_state(pipe, nullptr, nullptr, 5 * GST_SECOND);

    GstQuery *q = gst_query_new_latency();
    gboolean res = gst_element_query(pipe, q);
    if (res) {
        gboolean live;
        GstClockTime min_lat, max_lat;
        gst_query_parse_latency(q, &live, &min_lat, &max_lat);
        fail_unless(min_lat != GST_CLOCK_TIME_NONE,
                    "min latency must be valid");
    }
    gst_query_unref(q);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(pipe);
}

// Verify meta is preserved through the entire pipeline chain
// probe_element_name: attach probe to identity element for meta injection
inline void test_meta_preservation(const char *desc,
                                   const char *probe_element_name,
                                   const char *sink_name,
                                   int stream_id = 0) {
    GError *err = nullptr;
    GstElement *pipe = gst_parse_launch(desc, &err);
    fail_unless(pipe != nullptr);
    if (err) g_error_free(err);

    GstElement *adder = gst_bin_get_by_name(GST_BIN(pipe), probe_element_name);
    fail_unless(adder != nullptr, "element '%s' not found", probe_element_name);
    GstPad *src_pad = gst_element_get_static_pad(adder, "src");

    auto probe_cb = [](GstPad *, GstPadProbeInfo *info, gpointer ud)
        -> GstPadProbeReturn {
        int sid = GPOINTER_TO_INT(ud);
        GstBuffer *buf = GST_PAD_PROBE_INFO_BUFFER(info);
        buf = gst_buffer_make_writable(buf);
        GST_PAD_PROBE_INFO_DATA(info) = buf;
        if (!dx_get_frame_meta(buf)) {
            DXFrameMeta *fm = make_frame_meta(buf, sid, 64, 64);
            add_object_to_frame(fm, 1, 0.9f, 10, 10, 50, 50);
        }
        return GST_PAD_PROBE_OK;
    };
    gst_pad_add_probe(src_pad, GST_PAD_PROBE_TYPE_BUFFER,
                      probe_cb, GINT_TO_POINTER(stream_id), nullptr);
    gst_object_unref(src_pad);
    gst_object_unref(adder);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipe), sink_name);
    fail_unless(sink != nullptr, "sink '%s' not found", sink_name);

    int meta_count = 0;
    for (int i = 0; i < 20; i++) {
        GstSample *s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink),
                                                     2 * GST_SECOND);
        if (!s) break;
        GstBuffer *buf = gst_sample_get_buffer(s);
        DXFrameMeta *fm = dx_get_frame_meta(buf);
        if (fm) {
            fail_unless_equals_int(fm->_stream_id, stream_id);
            fail_unless(!fm->_object_meta_list.empty(),
                        "object_meta_list must not be empty after chain");
            meta_count++;
        }
        gst_sample_unref(s);
    }
    gst_object_unref(sink);

    gst_bus_timed_pop_filtered(bus, 10 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));

    fail_unless(meta_count > 0, "meta must survive pipeline chain");

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipe);
}

}  // namespace dxtest
