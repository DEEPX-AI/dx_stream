#pragma once
#include <vector>
#include <opencv2/opencv.hpp>
#include "./../metadata/gst-dxframemeta.hpp"
#include "./../metadata/gst-dxobjectmeta.hpp"

// YUV color structure
struct YUVColor {
    uint8_t y, u, v;
};

// Shared skeleton and color definitions for pose/OSD
extern const std::vector<std::vector<int>> skeleton;
extern const std::vector<cv::Scalar> pose_limb_color;
extern const std::vector<cv::Scalar> pose_kpt_color;
extern const std::vector<cv::Scalar> COLORS;

// BGR drawing functions
void draw_segmentation(cv::Mat &img, const DXObjectMeta *meta);
void draw_keypoints(cv::Mat &img, const DXObjectMeta *meta, float sx, float sy);
void draw_face(cv::Mat &img, const DXObjectMeta *meta, float sx, float sy);
void draw_label_or_id(cv::Mat &img, const DXObjectMeta *meta, float sx, float sy);
void draw_clip(cv::Mat &img, const DXObjectMeta *meta, bool v3_clip_text = false);
void draw_object_meta(cv::Mat &img, const DXObjectMeta *meta, float scale_x, float scale_y, bool v3_clip_text = false);

// YUV utility functions
YUVColor bgr_to_yuv_bt601(uint8_t b, uint8_t g, uint8_t r);

// YUV drawing functions for I420
void draw_rectangle_i420(uint8_t *y_plane, uint8_t *u_plane, uint8_t *v_plane,
                         int stride_y, int stride_uv, int width, int height,
                         int x1, int y1, int x2, int y2, YUVColor color, int thickness);
void draw_filled_rect_i420(uint8_t *y_plane, uint8_t *u_plane, uint8_t *v_plane,
                           int stride_y, int stride_uv, int width, int height,
                           int x1, int y1, int x2, int y2, YUVColor color);

// YUV drawing functions for NV12
void draw_rectangle_nv12(uint8_t *y_plane, uint8_t *uv_plane,
                         int stride_y, int stride_uv, int width, int height,
                         int x1, int y1, int x2, int y2, YUVColor color, int thickness);
void draw_filled_rect_nv12(uint8_t *y_plane, uint8_t *uv_plane,
                           int stride_y, int stride_uv, int width, int height,
                           int x1, int y1, int x2, int y2, YUVColor color);

// White text on Y plane only (for both I420 and NV12)
void draw_text_y_plane(uint8_t *y_plane, int stride, int width, int height,
                       const char *text, int x, int y, double scale);
void draw_keypoints_y_plane(uint8_t *y_plane, int stride, int width, int height,
                            const DXObjectMeta *meta, float sx, float sy);
void draw_face_y_plane(uint8_t *y_plane, int stride, int width, int height,
                       const DXObjectMeta *meta, float sx, float sy);
void draw_segmentation_i420(uint8_t *y_plane, uint8_t *u_plane, uint8_t *v_plane,
                            int stride_y, int stride_uv, int width, int height,
                            const DXObjectMeta *meta);
void draw_segmentation_nv12(uint8_t *y_plane, uint8_t *uv_plane,
                            int stride_y, int stride_uv, int width, int height,
                            const DXObjectMeta *meta);

// High-level YUV drawing functions
void draw_object_meta_yuv_i420(uint8_t *y_plane, uint8_t *u_plane, uint8_t *v_plane,
                               int stride_y, int stride_uv, int width, int height,
                               const DXObjectMeta *meta, float scale_x, float scale_y);

void draw_object_meta_yuv_nv12(uint8_t *y_plane, uint8_t *uv_plane,
                               int stride_y, int stride_uv, int width, int height,
                               const DXObjectMeta *meta, float scale_x, float scale_y);