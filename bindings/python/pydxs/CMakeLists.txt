cmake_minimum_required(VERSION 3.16)
project(pydxs_module)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(Python COMPONENTS Interpreter Development REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0)
pkg_check_modules(GLIB REQUIRED glib-2.0)

# Set PROJECT_ROOT
if(NOT DEFINED ENV{PROJECT_ROOT})
    get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
    message(STATUS "Auto-detected PROJECT_ROOT: ${PROJECT_ROOT}")
else()
    set(PROJECT_ROOT $ENV{PROJECT_ROOT})
    message(STATUS "Using PROJECT_ROOT from environment: ${PROJECT_ROOT}")
endif()

# Find pybind11 (pip-installed)
find_package(pybind11 CONFIG REQUIRED)

# Find libgstdxstream.so
find_library(GSTDXSTREAM_LIB gstdxstream
    PATHS "${PROJECT_ROOT}/install/lib/gstreamer-1.0"
    NO_DEFAULT_PATH REQUIRED)
if(NOT GSTDXSTREAM_LIB)
    message(FATAL_ERROR "libgstdxstream.so not found! Run ./build.sh first.")
endif()

# Build Python module
pybind11_add_module(pydxs src/metadata_binding.cpp)

target_include_directories(pydxs PRIVATE
    ${GST_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    "${PROJECT_ROOT}/gst-dxstream-plugin/metadata"
    "${PROJECT_ROOT}/gst-dxstream-plugin/general")

target_link_libraries(pydxs PRIVATE
    ${GST_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${GSTDXSTREAM_LIB})

# Install
install(TARGETS pydxs DESTINATION .)
