// L0 SPY infra -- capture release/copy callback call counts and arguments
//
// Usage:
//   dxtest::reset_spy();
//   ... register callbacks + perform operations ...
//   fail_unless_equals_int(dxtest::g_spy_release_count.load(), 1);
//
// Key: all callback calls are tracked by atomic counters. Last argument pointers are also captured.
// For header-only use, avoid anonymous-namespace variables instead of inline variables (C++17),
// and ensure this header is included only once per test binary.

#pragma once

#include <glib.h>
#include <atomic>
#include <cstring>

namespace dxtest {

struct SpyPayload {
    int magic;
    char tag[32];
};

// Include in a single compilation unit only -- may conflict when linking multiple TUs (intentional).
static std::atomic<int> g_spy_release_count{0};
static std::atomic<int> g_spy_copy_count{0};
static std::atomic<gpointer> g_spy_last_release_arg{nullptr};
static std::atomic<gpointer> g_spy_last_copy_src{nullptr};
static std::atomic<gpointer> g_spy_last_copy_result{nullptr};

inline void reset_spy() {
    g_spy_release_count.store(0);
    g_spy_copy_count.store(0);
    g_spy_last_release_arg.store(nullptr);
    g_spy_last_copy_src.store(nullptr);
    g_spy_last_copy_result.store(nullptr);
}

inline SpyPayload *new_spy_payload(int magic, const char *tag) {
    auto *p = (SpyPayload *)g_malloc0(sizeof(SpyPayload));
    p->magic = magic;
    std::strncpy(p->tag, tag ? tag : "", sizeof(p->tag) - 1);
    return p;
}

inline void spy_release_cb(gpointer data) {
    g_spy_last_release_arg.store(data);
    g_spy_release_count.fetch_add(1);
    g_free(data);
}

inline gpointer spy_copy_cb(gpointer src) {
    g_spy_last_copy_src.store(src);
    auto *s = (const SpyPayload *)src;
    auto *d = (SpyPayload *)g_malloc0(sizeof(SpyPayload));
    *d = *s;
    g_spy_last_copy_result.store(d);
    g_spy_copy_count.fetch_add(1);
    return d;
}

}  // namespace dxtest
