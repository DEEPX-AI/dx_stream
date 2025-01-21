#include "gst-dxmeta.hpp"
#include <gst/gst.h>
#include <gst/video/video.h>
#include <libyuv.h>
#include <opencv2/opencv.hpp>

cv::Mat convert_color(DXFrameMeta *frame_meta, gchar *format = "RGB");
void SurfaceToOrigin(DXFrameMeta *frame_meta);
void set_surface(DXFrameMeta *frame_meta);

void I420ToRGBA(GstBuffer *buffer, uint8_t *dst, int width, int height);
void RGBToRGBA(GstBuffer *buffer, uint8_t *dst, int width, int height);
void NV12ToRGBA(GstBuffer *buffer, uint8_t *dst, int width, int height);
