#ifndef DX_GETOPT_H
#define DX_GETOPT_H

// dx_getopt: minimal getopt for Windows console apps
//
// Supported:
//   - Single-character options: -x
//   - Required argument via ':' in optstring: "f:" means -f <value> or -f<value>
//   - Returns '?' for unknown options or missing arguments
//   - Returns -1 when all options are consumed
//
// NOT supported (use GLib GOptionContext if you need these):
//   - Leading ':' in optstring (silent error mode)
//   - '::' optional arguments
//   - '--' end-of-options terminator
//   - Bundled short options (-abc)
//   - Long options (--foo)
//
// Usage:
//   #include "common/dx_getopt.h"
//   // Windows: uses the inline implementation below
//   // Linux/macOS: includes <unistd.h> which provides POSIX getopt

#ifdef _WIN32

#include <string.h>

static int   dx_optind = 1;
static char *dx_optarg = NULL;

static inline int dx_getopt(int argc, char *argv[], const char *optstring) {
    if (dx_optind >= argc) return -1;

    char *arg = argv[dx_optind];
    if (arg[0] != '-' || arg[1] == '\0') return -1;

    const char *p = strchr(optstring, arg[1]);
    if (!p) {
        dx_optind++;
        return '?';
    }

    dx_optind++;
    dx_optarg = NULL;
    if (p[1] == ':') {
        if (arg[2] != '\0') {
            dx_optarg = &arg[2];  // attached form: -fvalue
        } else if (dx_optind >= argc) {
            return '?';
        } else {
            dx_optarg = argv[dx_optind++];  // separated form: -f value
        }
    }

    return arg[1];
}

#else
#include <unistd.h>
#endif

#endif /* DX_GETOPT_H */
