#pragma once
#include <vector>
#include <opencv2/opencv.hpp>
#include "./../metadata/gst-dxframemeta.hpp"
#include "./../metadata/gst-dxobjectmeta.hpp"

// Shared skeleton and color definitions for pose/OSD
extern const std::vector<std::vector<int>> skeleton;
extern const std::vector<cv::Scalar> pose_limb_color;
extern const std::vector<cv::Scalar> pose_kpt_color;
extern const std::vector<cv::Scalar> COLORS;

void draw_segmentation(cv::Mat &img, const DXObjectMeta *meta);
void draw_keypoints(cv::Mat &img, const DXObjectMeta *meta, float sx, float sy);
void draw_face(cv::Mat &img, const DXObjectMeta *meta, float sx, float sy);
void draw_label_or_id(cv::Mat &img, const DXObjectMeta *meta, float sx, float sy);
void draw_clip(cv::Mat &img, const DXObjectMeta *meta, bool v3_clip_text = false);
void draw_object_meta(cv::Mat &img, const DXObjectMeta *meta, float scale_x, float scale_y, bool v3_clip_text = false);