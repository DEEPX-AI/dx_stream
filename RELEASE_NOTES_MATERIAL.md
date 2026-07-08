## PR 192
### 1. Changed
- **Cross-platform compatibility**: guarded POSIX-only APIs with #ifdef _WIN32 across the plugin source (no behavioral change on Linux) [SR-452](https://deepx.atlassian.net/browse/SR-452)
### 2. Fixed
- Missing finalize method in `GstDxInputSelector` and `GstDxOutputSelector` causing resource leak
- mp4 extension not handled in `benchmark/config.py` 
### 3. Added
- **Windows Build and Runtime Environment**: Full Windows MSVC support including dependency check, build, demo launcher, test suite, Python binding (pydxs), and build guide documentation [SR-452](https://deepx.atlassian.net/browse/SR-452)
