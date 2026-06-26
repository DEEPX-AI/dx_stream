// L2 Pipeline integration test helper
// Used in: base/pipeline/test_pl_*.cpp
//
// Bundles gst_parse_launch / manual element build + GstBus watcher + appsink capture.

#pragma once

#include <gst/app/gstappsink.h>
#include <gst/check/gstcheck.h>
#include <gst/gst.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace dxtest {

// ---------------------------------------------------------------------------
// Bus message accumulator
// ---------------------------------------------------------------------------

struct BusRecord {
    std::vector<GstMessage *> errors;
    std::vector<GstMessage *> warnings;
    std::vector<GstMessage *> infos;
    bool got_eos = false;
    bool got_async_done = false;

    ~BusRecord() {
        for (auto *m : errors)   gst_message_unref(m);
        for (auto *m : warnings) gst_message_unref(m);
        for (auto *m : infos)    gst_message_unref(m);
    }
};

// ---------------------------------------------------------------------------
// Pipeline fixture
// ---------------------------------------------------------------------------

struct Pipeline {
    GstElement *pipeline = nullptr;
    GstBus *bus = nullptr;
    GMainLoop *loop = nullptr;
    guint bus_watch_id = 0;
    guint timeout_id = 0;
    BusRecord rec;

    Pipeline() { loop = g_main_loop_new(nullptr, FALSE); }

    ~Pipeline() {
        if (timeout_id)   { g_source_remove(timeout_id); timeout_id = 0; }
        if (bus_watch_id) { g_source_remove(bus_watch_id); bus_watch_id = 0; }
        if (pipeline)     { gst_element_set_state(pipeline, GST_STATE_NULL);
                            gst_object_unref(pipeline); pipeline = nullptr; }
        if (bus)          { gst_object_unref(bus); bus = nullptr; }
        if (loop)         { g_main_loop_unref(loop); loop = nullptr; }
    }

    void setup_from_string(const char *desc) {
        GError *err = nullptr;
        pipeline = gst_parse_launch(desc, &err);
        if (err) {
            std::string msg = err->message ? err->message : "(null)";
            g_error_free(err);
            fail("gst_parse_launch failed: %s", msg.c_str());
        }
        fail_unless(pipeline != nullptr);
        bus = gst_element_get_bus(pipeline);
        bus_watch_id = gst_bus_add_watch(bus, &Pipeline::bus_cb, this);
    }

    void setup_empty(const char *name = "test-pipeline") {
        pipeline = gst_pipeline_new(name);
        bus = gst_element_get_bus(pipeline);
        bus_watch_id = gst_bus_add_watch(bus, &Pipeline::bus_cb, this);
    }

    void play() {
        GstStateChangeReturn r = gst_element_set_state(pipeline, GST_STATE_PLAYING);
        fail_unless(r != GST_STATE_CHANGE_FAILURE, "set_state PLAYING failed");
    }

    void run_until_eos_or_timeout(guint sec) {
        timeout_id = g_timeout_add_seconds(sec, &Pipeline::to_cb, this);
        g_main_loop_run(loop);
    }

    // Use when EOS will not reach the bus (e.g. dxoutputselector drops global EOS)
    void run_for(guint sec) {
        timeout_id = g_timeout_add_seconds(sec, &Pipeline::to_cb, this);
        g_main_loop_run(loop);
    }

    GstElement *by_name(const char *name) {
        return gst_bin_get_by_name(GST_BIN(pipeline), name);
    }

    bool had_error() const { return !rec.errors.empty(); }

private:
    static gboolean bus_cb(GstBus *, GstMessage *m, gpointer d) {
        auto *self = static_cast<Pipeline *>(d);
        switch (GST_MESSAGE_TYPE(m)) {
            case GST_MESSAGE_EOS:
                self->rec.got_eos = true;
                g_main_loop_quit(self->loop);
                break;
            case GST_MESSAGE_ERROR:
                self->rec.errors.push_back(gst_message_ref(m));
                g_main_loop_quit(self->loop);
                break;
            case GST_MESSAGE_WARNING:
                self->rec.warnings.push_back(gst_message_ref(m));
                break;
            case GST_MESSAGE_INFO:
                self->rec.infos.push_back(gst_message_ref(m));
                break;
            case GST_MESSAGE_ASYNC_DONE:
                self->rec.got_async_done = true;
                break;
            default: break;
        }
        return TRUE;
    }
    static gboolean to_cb(gpointer d) {
        auto *self = static_cast<Pipeline *>(d);
        self->timeout_id = 0;
        g_main_loop_quit(self->loop);
        return G_SOURCE_REMOVE;
    }
};

// ---------------------------------------------------------------------------
// Error inspection
// ---------------------------------------------------------------------------

struct ErrorInfo {
    GQuark domain = 0;
    int code = 0;
    std::string message;
    std::string debug;
};

inline ErrorInfo parse_error(GstMessage *m) {
    ErrorInfo r;
    GError *err = nullptr;
    gchar *dbg = nullptr;
    gst_message_parse_error(m, &err, &dbg);
    if (err) { r.domain = err->domain; r.code = err->code;
               r.message = err->message ? err->message : "";
               g_error_free(err); }
    if (dbg) { r.debug = dbg; g_free(dbg); }
    return r;
}

// ---------------------------------------------------------------------------
// appsink buffer collector
// ---------------------------------------------------------------------------

struct AppSinkCollector {
    std::vector<GstSample *> samples;
    GstElement *sink = nullptr;

    static AppSinkCollector *attach(GstElement *appsink) {
        auto *c = new AppSinkCollector();
        c->sink = appsink;
        g_object_set(appsink, "emit-signals", TRUE, "sync", FALSE,
                     "max-buffers", (guint)256, "drop", FALSE, nullptr);
        g_signal_connect(appsink, "new-sample",
                         G_CALLBACK(&AppSinkCollector::on_new_sample), c);
        return c;
    }

    ~AppSinkCollector() {
        for (auto *s : samples) gst_sample_unref(s);
    }

private:
    static GstFlowReturn on_new_sample(GstAppSink *s, gpointer d) {
        auto *self = static_cast<AppSinkCollector *>(d);
        GstSample *sample = gst_app_sink_pull_sample(s);
        if (sample) self->samples.push_back(sample);
        return GST_FLOW_OK;
    }
};

}  // namespace dxtest
