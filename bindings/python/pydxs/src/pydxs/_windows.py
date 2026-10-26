"""Resolve the selected DX Stream and SDK runtime directories on Windows."""

import os
from pathlib import Path
import sys


DEFAULT_GSTREAMER_ROOTS = (
    r"C:\Program Files\gstreamer\1.0\msvc_x86_64",
    r"C:\gstreamer\1.0\msvc_x86_64",
)


def _absolute_path(value, setting):
    value = os.path.expandvars(str(value).strip().strip('"'))
    if not os.path.isabs(value):
        raise ImportError("pydxs: {} must be an absolute path: {!r}".format(setting, value))
    return Path(os.path.normpath(value))


def _has_plugin(prefix):
    return any((directory / "gstdxstream.dll").is_file()
               for directory in (prefix / "lib" / "gstreamer-1.0", prefix / "bin"))


def _install_prefix(paths):
    root = os.environ.get("DX_STREAM_ROOT")
    if root:
        prefix = _absolute_path(root, "DX_STREAM_ROOT") / "install"
        if not _has_plugin(prefix):
            raise ImportError(
                "pydxs: DX_STREAM_ROOT does not contain install/lib/gstreamer-1.0/"
                "gstdxstream.dll or install/bin/gstdxstream.dll: {}. "
                "Run build.bat in the selected checkout.".format(prefix)
            )
        return prefix

    candidates = [Path(sys.prefix).parent / "install"]
    candidates.extend(parent / "install" for parent in Path(__file__).resolve().parents)
    if paths.get("install_prefix"):
        candidates.append(_absolute_path(paths["install_prefix"], "build install prefix"))
    for prefix in candidates:
        if _has_plugin(prefix):
            return prefix
    raise ImportError(
        "pydxs: gstdxstream.dll was not found. Run build.bat and set DX_STREAM_ROOT "
        "to the absolute dx_stream checkout path if using an external venv."
    )


def _gstreamer_bin(paths):
    candidates = (
        os.environ.get("GSTREAMER_1_0_ROOT_MSVC_X86_64"),
        os.environ.get("GSTREAMER_1_0_ROOT_X86_64"),
        paths.get("gstreamer_root"),
    ) + DEFAULT_GSTREAMER_ROOTS
    for candidate in candidates:
        if not candidate:
            continue
        directory = _absolute_path(candidate, "GStreamer root") / "bin"
        if all((directory / executable).is_file()
               for executable in ("gst-inspect-1.0.exe", "gst-launch-1.0.exe")):
            return directory
    raise ImportError(
        "pydxs: GStreamer MSVC x64 runtime was not found. Set "
        "GSTREAMER_1_0_ROOT_MSVC_X86_64 (or GSTREAMER_1_0_ROOT_X86_64) to its "
        "absolute installation root. Install it from https://gstreamer.freedesktop.org/download/."
    )


def _sdk_bin(paths, variable, key, optional=False):
    root = os.environ.get(variable) or paths.get(key)
    if root:
        directory = _absolute_path(root, variable) / "bin"
        if not directory.is_dir():
            raise ImportError(
                "pydxs: {} runtime directory does not exist: {}. "
                "Set {} to the installed SDK root.".format(variable, directory, variable)
            )
        return directory
    if optional:
        return None
    legacy = Path(r"C:\Program Files\DEEPX\DXNN\runtime")
    if legacy.is_dir():
        return legacy
    raise ImportError(
        "pydxs: DEEPX runtime was not found. Install the runtime and set "
        "DEEPX_SDK_DIR to its absolute SDK root (containing bin)."
    )


def dll_directories(paths):
    prefix = _install_prefix(paths)
    candidates = [
        prefix / "lib" / "gstreamer-1.0", prefix / "bin",
        _gstreamer_bin(paths), _sdk_bin(paths, "DEEPX_SDK_DIR", "deepx_sdk_dir"),
        _sdk_bin(paths, "DEEPX_VNPU_DIR", "deepx_vnpu_dir", optional=True),
    ]
    directories = []
    seen = set()
    for directory in candidates:
        if directory is None or not directory.is_dir():
            continue
        normalized = os.path.normpath(str(directory))
        key = os.path.normcase(normalized)
        if key not in seen:
            seen.add(key)
            directories.append(normalized)
    return directories


def register_dll_directories(paths):
    directories = dll_directories(paths)
    register = getattr(os, "add_dll_directory", None)
    if register is None:
        if sys.version_info >= (3, 8):
            raise ImportError("pydxs: Windows Python 3.8+ requires os.add_dll_directory().")
        os.environ["PATH"] = os.pathsep.join(directories + [os.environ.get("PATH", "")])
        return [], directories

    handles = []
    try:
        for directory in directories:
            handles.append(register(directory))
    except OSError as error:
        for handle in handles:
            handle.close()
        raise ImportError(
            "pydxs: failed to register DLL directory {}: {}".format(directory, error)
        ) from error
    return handles, directories
