// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef LUNDUKEPAINT_RASTER_EFFECTS_HPP
#define LUNDUKEPAINT_RASTER_EFFECTS_HPP

#include <cstdint>

namespace lundukepaint {

// In-place adjustments. Alpha is left unchanged.
void invert_rgba(std::uint8_t* rgba, int width, int height, int stride);
void grayscale_rgba(std::uint8_t* rgba, int width, int height, int stride);

// brightness and contrast are -100..100.
void brightness_contrast_rgba(std::uint8_t* rgba, int width, int height, int stride,
                              int brightness, int contrast);

// hue is -180..180 degrees; saturation is -100..100.
void hue_saturation_rgba(std::uint8_t* rgba, int width, int height, int stride, int hue,
                         int saturation);

// levels is 2..16.
void posterize_rgba(std::uint8_t* rgba, int width, int height, int stride, int levels);

// src and dest must not alias. radius is 1..16.
void box_blur_rgba(const std::uint8_t* src, int width, int height, int src_stride,
                   std::uint8_t* dest, int dest_stride, int radius);
void sharpen_rgba(const std::uint8_t* src, int width, int height, int src_stride,
                  std::uint8_t* dest, int dest_stride);
void emboss_rgba(const std::uint8_t* src, int width, int height, int src_stride,
                 std::uint8_t* dest, int dest_stride);

}  // namespace lundukepaint

#endif
