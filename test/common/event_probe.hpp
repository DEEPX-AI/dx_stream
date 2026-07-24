// Event probe helper — pad probe for counting/tracking GStreamer events
#pragma once

#include <gst/gst.h>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace dxtest {

struct EventTrace {
    std::mutex mu;
    std::map<GstEventType, int> counts;
    std::vector<GstEventType> order;

    static GstPadProbeReturn probe_cb(GstPad *, GstPadProbeInfo *info,
                                       gpointer ud) {
        auto *self = static_cast<EventTrace *>(ud);
        GstEvent *ev = GST_PAD_PROBE_INFO_EVENT(info);
        GstEventType t = GST_EVENT_TYPE(ev);
        std::lock_guard<std::mutex> lk(self->mu);
        self->counts[t]++;
        self->order.push_back(t);
        return GST_PAD_PROBE_OK;
    }

    void attach(GstPad *pad) {
        gst_pad_add_probe(pad,
            (GstPadProbeType)(GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM |
                              GST_PAD_PROBE_TYPE_EVENT_UPSTREAM),
            probe_cb, this, nullptr);
    }

    void attach_downstream(GstPad *pad) {
        gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
                          probe_cb, this, nullptr);
    }

    int count(GstEventType t) {
        std::lock_guard<std::mutex> lk(mu);
        auto it = counts.find(t);
        return (it != counts.end()) ? it->second : 0;
    }

    bool has(GstEventType t) { return count(t) > 0; }

    bool order_before(GstEventType a, GstEventType b) {
        std::lock_guard<std::mutex> lk(mu);
        int ia = -1, ib = -1;
        for (int i = 0; i < (int)order.size(); i++) {
            if (ia < 0 && order[i] == a) ia = i;
            if (ib < 0 && order[i] == b) ib = i;
        }
        return ia >= 0 && ib >= 0 && ia < ib;
    }
};

}  // namespace dxtest
