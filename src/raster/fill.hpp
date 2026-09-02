// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_RASTER_FILL_HPP
#define BRUSHPAD_RASTER_FILL_HPP

#include "raster/types.hpp"

#include <cstdint>
#include <vector>

namespace brushpad {

// Contiguous flood fill. tolerance is Chebyshev distance in RGBA (0 = exact).
// Writes into rgba (straight alpha, stride = width * 4 unless given).
void flood_fill(std::uint8_t* rgba, int width, int height, int stride, int x, int y,
                Color replacement, int tolerance, Rect* dirty);

// Contiguous color select (Chebyshev), writes 0/255 mask of size width*height.
// *bounds is the tight rectangle of selected pixels (empty if none).
void flood_mask(const std::uint8_t* rgba, int width, int height, int stride, int x, int y,
                int tolerance, std::vector<std::uint8_t>& mask, Rect* bounds);

}  // namespace brushpad

#endif
