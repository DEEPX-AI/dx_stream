#include "gst-dxmeta.hpp"
#include <gst/gst.h>
#include <gst/video/video.h>
#include <libyuv.h>
#include <opencv2/opencv.hpp>

uint8_t *Resize(uint8_t *src, int src_width, int src_height, int dst_width,
                int dst_height, const gchar *format);
uint8_t *Resize(GstBuffer *buf, int src_width, int src_height, int dst_width,
                int dst_height, const gchar *format);
uint8_t *CvtColor(uint8_t *src, int width, int height, const gchar *src_format,
                  const gchar *dst_format);
uint8_t *CvtColor(GstBuffer *buf, int width, int height,
                  const gchar *src_format, const gchar *dst_format);
uint8_t *Crop(GstBuffer *buf, int src_width, int src_height, int crop_x,
              int crop_y, int crop_width, int crop_height, const gchar *format);

void SurfaceToOrigin(DXFrameMeta *frame_meta, uint8_t *surface);
void RGB24toNV12(DXFrameMeta *frame_meta, uint8_t *surface);
void RGB24toI420(DXFrameMeta *frame_meta, uint8_t *surface);
