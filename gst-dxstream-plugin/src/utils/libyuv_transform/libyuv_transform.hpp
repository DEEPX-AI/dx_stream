#ifndef LIBYUV_TRANSFORM_HPP
#define LIBYUV_TRANSFORM_HPP

#include <gst/gst.h>
#include <gst/video/video.h>
#include <libyuv.h>
#include <opencv2/opencv.hpp>
#include <vector>

// ============================================================================
// RAII-based API using std::vector for automatic memory management
// ============================================================================

/**
 * @brief Crop a region from a GstBuffer
 * @param buf Input GstBuffer
 * @param input_info Video info for the input buffer (can be nullptr)
 * @param dst Output vector (automatically resized)
 * @param src_width Source width
 * @param src_height Source height
 * @param crop_x Crop starting X coordinate
 * @param crop_y Crop starting Y coordinate
 * @param crop_width Crop width
 * @param crop_height Crop height
 * @param format Color format ("RGB", "I420", "NV12")
 */
void Crop(GstBuffer *buf, const GstVideoInfo *input_info, std::vector<uint8_t>& dst, 
          int src_width, int src_height, int crop_x, int crop_y, 
          int crop_width, int crop_height, const gchar *format);

/**
 * @brief Resize an image (vector to vector)
 * @param src Source image data
 * @param dst Destination vector (automatically resized)
 * @param src_width Source width
 * @param src_height Source height
 * @param dst_width Destination width
 * @param dst_height Destination height
 * @param format Color format ("RGB", "I420", "NV12")
 */
void Resize(std::vector<uint8_t>& src, std::vector<uint8_t>& dst, 
            int src_width, int src_height, int dst_width, int dst_height, 
            const gchar *format);

/**
 * @brief Resize an image from GstBuffer
 * @param buf Input GstBuffer
 * @param input_info Video info for the input buffer (can be nullptr)
 * @param dst Destination vector (automatically resized)
 * @param src_width Source width
 * @param src_height Source height
 * @param dst_width Destination width
 * @param dst_height Destination height
 * @param format Color format ("RGB", "I420", "NV12")
 */
void Resize(GstBuffer *buf, const GstVideoInfo *input_info, std::vector<uint8_t>& dst, 
            int src_width, int src_height, int dst_width, int dst_height, 
            const gchar *format);

/**
 * @brief Convert color format (vector to vector)
 * @param src Source image data
 * @param dst Destination vector (automatically resized)
 * @param width Image width
 * @param height Image height
 * @param src_format Source color format
 * @param dst_format Destination color format
 */
void CvtColor(std::vector<uint8_t>& src, std::vector<uint8_t>& dst, 
              int width, int height, const gchar *src_format, 
              const gchar *dst_format);

/**
 * @brief Convert color format from GstBuffer
 * @param buf Input GstBuffer
 * @param input_info Video info for the input buffer (can be nullptr)
 * @param dst Destination vector (automatically resized)
 * @param width Image width
 * @param height Image height
 * @param src_format Source color format
 * @param dst_format Destination color format
 */
void CvtColor(GstBuffer *buf, const GstVideoInfo *input_info, std::vector<uint8_t>& dst, 
              int width, int height, const gchar *src_format, 
              const gchar *dst_format);

#endif // LIBYUV_TRANSFORM_HPP