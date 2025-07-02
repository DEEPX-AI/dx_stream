#include "gst-dxmeta.hpp"
#include <gst/gst.h>
#include <gst/video/video.h>
#include <libyuv.h>
#include <opencv2/opencv.hpp>

void Resize(uint8_t *src, uint8_t **dst, int src_width, int src_height, int dst_width,
                int dst_height, const gchar *format);
void Resize(GstBuffer *buf, uint8_t **dst, int src_width, int src_height, int dst_width,
                int dst_height, const gchar *format);
void CvtColor(uint8_t *src, uint8_t **dst, int width, int height, const gchar *src_format,
                  const gchar *dst_format);
void CvtColor(GstBuffer *buf, uint8_t **dst, int width, int height,
                  const gchar *src_format, const gchar *dst_format);
void Crop(GstBuffer *buf, uint8_t **dst, int src_width, int src_height, int crop_x,
              int crop_y, int crop_width, int crop_height, const gchar *format);

void SurfaceToOrigin(DXFrameMeta *frame_meta, uint8_t *surface);
void RGB24toNV12(DXFrameMeta *frame_meta, uint8_t *surface);
void RGB24toI420(DXFrameMeta *frame_meta, uint8_t *surface);
