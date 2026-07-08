#ifndef DX_DLFCN_H
#define DX_DLFCN_H

#ifdef _WIN32
#include <stdio.h>
#include <windows.h>

#ifndef RTLD_LAZY
#define RTLD_LAZY 0
#endif
#ifndef RTLD_NOW
#define RTLD_NOW 0
#endif
#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0
#endif
#ifndef RTLD_LOCAL
#define RTLD_LOCAL 0
#endif

// dx_dlfcn: POSIX dlfcn shim for Windows
//
// Semantics: dlerror() returns NULL when no error has occurred since the
//            last dlerror() call, matching POSIX.1-2008.
// Thread safety: each thread has its own error state (thread_local).
// Linkage: selectany ensures a single instance across all TUs in the same
//          binary, matching POSIX's global-per-thread dlerror state.
//          (C++14 equivalent of C++17 inline variables on MSVC/MinGW.)

__declspec(selectany) thread_local DWORD  dx__dl_errcode  = 0;
__declspec(selectany) thread_local int    dx__dl_has_error = 0;
__declspec(selectany) thread_local char   dx__dl_errbuf[256] = {0};

static inline void dx__dl_set_error(int is_failure) {
    if (is_failure) {
        dx__dl_errcode  = GetLastError();
        dx__dl_has_error = 1;
    } else {
        dx__dl_has_error = 0;
    }
}

static inline void *dlopen(const char *path, int mode) {
    (void)mode;
    SetLastError(0);
    HMODULE h = LoadLibraryA(path);
    dx__dl_set_error(h == NULL);
    return (void *)h;
}

static inline void *dlsym(void *handle, const char *name) {
    SetLastError(0);
    FARPROC p = GetProcAddress((HMODULE)handle, name);
    dx__dl_set_error(p == NULL);
#ifdef _MSC_VER
#pragma warning(suppress : 4191)  // FARPROC→void* (unavoidable for POSIX compat)
#endif
    return (void *)(void(*)(void))p;
}

static inline int dlclose(void *handle) {
    SetLastError(0);
    BOOL ok = FreeLibrary((HMODULE)handle);
    dx__dl_set_error(!ok);
    return ok ? 0 : -1;
}

static inline const char *dlerror(void) {
    if (!dx__dl_has_error)
        return NULL;

    dx__dl_has_error = 0;

    DWORD len = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, dx__dl_errcode, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
        dx__dl_errbuf, sizeof(dx__dl_errbuf) - 1, NULL);

    if (len == 0) {
        snprintf(dx__dl_errbuf, sizeof(dx__dl_errbuf),
                 "Win32 error %lu", (unsigned long)dx__dl_errcode);
    } else {
        while (len > 0 && (dx__dl_errbuf[len - 1] == '\r' ||
                           dx__dl_errbuf[len - 1] == '\n'))
            dx__dl_errbuf[--len] = '\0';
    }

    dx__dl_errcode = 0;
    return dx__dl_errbuf;
}

#else
#include <dlfcn.h>
#endif

#endif /* DX_DLFCN_H */
