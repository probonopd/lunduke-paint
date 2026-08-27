// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_RASTER_TEXT_HPP
#define BRUSHPAD_RASTER_TEXT_HPP

#include "raster/types.hpp"

#include <cstdint>

namespace brushpad {

// Blit a straight-alpha RGBA bitmap onto dest. skip_transparent leaves dest
// where src alpha is 0. Used by the text tool after Pango rasterizes glyphs.
void blit_rgba_buffer(std::uint8_t* dest, int dw, int dh, int dstride, int dx, int dy,
                      const std::uint8_t* src, int sw, int sh, int sstride, bool skip_transparent,
                      Rect* dirty);

}  // namespace brushpad

#endif
