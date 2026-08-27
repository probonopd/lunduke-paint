// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_RASTER_TRANSFORM_HPP
#define BRUSHPAD_RASTER_TRANSFORM_HPP

#include "raster/types.hpp"

#include <cstdint>

namespace brushpad {

void flip_h(std::uint8_t* rgba, int width, int height, int stride);
void flip_v(std::uint8_t* rgba, int width, int height, int stride);
void rotate_180(std::uint8_t* rgba, int width, int height, int stride);

// dest size must be (src_height × src_width). dest(x,y) = src(y, src_h-1-x).
void rotate_90_cw(const std::uint8_t* src, int src_w, int src_h, int src_stride,
                  std::uint8_t* dest, int dest_stride);
void rotate_90_ccw(const std::uint8_t* src, int src_w, int src_h, int src_stride,
                   std::uint8_t* dest, int dest_stride);

void scale_nearest(const std::uint8_t* src, int src_w, int src_h, int src_stride,
                   std::uint8_t* dest, int dest_w, int dest_h, int dest_stride);
void scale_bilinear(const std::uint8_t* src, int src_w, int src_h, int src_stride,
                    std::uint8_t* dest, int dest_w, int dest_h, int dest_stride);

void resize_canvas(const std::uint8_t* src, int src_w, int src_h, int src_stride,
                   std::uint8_t* dest, int dest_w, int dest_h, int dest_stride, Color fill);

void crop_rect(const std::uint8_t* src, int src_w, int src_h, int src_stride, Rect rect,
               std::uint8_t* dest, int dest_stride);

// Trim uniform-color borders. Returns a 1×1 rect if the whole buffer is uniform.
Rect autocrop_bounds(const std::uint8_t* rgba, int width, int height, int stride);

}  // namespace brushpad

#endif
