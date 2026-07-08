// Shared across all layers -- simple GstBus message collector
// Independent of the Pipeline struct in pipeline_helpers.hpp,
// a lightweight watcher for running elements directly in a bin.

#pragma once

#include <gst/gst.h>
#include <mutex>
#include <vector>

namespace dxtest {

struct BusWatcher {
    GstBus *bus = nullptr;
    guint id = 0;
    std::mutex mu;
    std::vector<GstMessage *> errors;
    std::vector<GstMessage *> warnings;
    bool got_eos = false;

    explicit BusWatcher(GstElement *pipeline_or_bin) {
        bus = gst_element_get_bus(pipeline_or_bin);
        id = gst_bus_add_watch(bus, &BusWatcher::cb, this);
    }

    ~BusWatcher() {
        if (id) g_source_remove(id);
        if (bus) gst_object_unref(bus);
        std::lock_guard<std::mutex> g(mu);
        for (auto *m : errors)   gst_message_unref(m);
        for (auto *m : warnings) gst_message_unref(m);
    }

    bool had_error() {
        std::lock_guard<std::mutex> g(mu);
        return !errors.empty();
    }

private:
    static gboolean cb(GstBus *, GstMessage *m, gpointer d) {
        auto *self = static_cast<BusWatcher *>(d);
        std::lock_guard<std::mutex> g(self->mu);
        switch (GST_MESSAGE_TYPE(m)) {
            case GST_MESSAGE_ERROR:   self->errors.push_back(gst_message_ref(m)); break;
            case GST_MESSAGE_WARNING: self->warnings.push_back(gst_message_ref(m)); break;
            case GST_MESSAGE_EOS:     self->got_eos = true; break;
            default: break;
        }
        return TRUE;
    }
};

}  // namespace dxtest
