"""Python bindings for DX Stream metadata with Windows DLL discovery."""

from importlib import import_module as _import_module

from ._runtime_paths import RUNTIME_PATHS as _runtime_paths
from ._windows import register_dll_directories as _register_dll_directories


# Keep the directory handles alive for later native calls and dependent imports.
_dll_handles, _dll_directories = _register_dll_directories(_runtime_paths)
try:
    _native = _import_module("._pydxs", __name__)
except ImportError as _error:
    for _handle in _dll_handles:
        _handle.close()
    _dll_handles.clear()
    raise ImportError(
        "pydxs: could not load the native binding: {}. "
        "DLL directories: {}. Ensure DX Stream, GStreamer and the DEEPX runtime "
        "are installed for MSVC x64 and match this Python version. "
        "Run build.bat to rebuild the binding.".format(_error, "; ".join(_dll_directories))
    ) from _error

__doc__ = _native.__doc__
__all__ = [name for name in dir(_native) if not name.startswith("_")]
globals().update({name: getattr(_native, name) for name in __all__})
