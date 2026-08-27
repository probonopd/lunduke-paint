// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_RASTER_BLEND_HPP
#define BRUSHPAD_RASTER_BLEND_HPP

#include "raster/types.hpp"

#include <cstdint>

namespace brushpad {

enum class BlendMode {
  Normal = 0,
  Multiply,
  Screen,
  Overlay,
  Darken,
  Lighten
};

constexpr int kBlendModeCount = 6;

const char* blend_mode_label(BlendMode mode);
const char* blend_mode_ora_op(BlendMode mode);
BlendMode blend_mode_from_ora(const char* composite_op);
BlendMode blend_mode_from_index(int index);

// Blend one straight-alpha RGBA8888 source pixel onto dest (in place).
void blend_pixel(std::uint8_t* dest, const std::uint8_t* src, BlendMode mode, float opacity);

// Composite src onto dest for the given destination rectangle. dest is
// viewport-sized (dest_w × dest_h). src is a full layer buffer. src is
// sampled at (x - offset_x, y - offset_y) in document space.
void blend_layer_rect(std::uint8_t* dest, int dest_w, int dest_h, int dest_stride,
                      const std::uint8_t* src, int src_w, int src_h, int src_stride,
                      int offset_x, int offset_y, Rect dest_doc, BlendMode mode, float opacity);

}  // namespace brushpad

#endif
