// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_RASTER_FILL_HPP
#define BRUSHPAD_RASTER_FILL_HPP

#include "raster/types.hpp"

#include <cstdint>

namespace brushpad {

// Contiguous flood fill. tolerance is Chebyshev distance in RGBA (0 = exact).
// Writes into rgba (straight alpha, stride = width * 4 unless given).
void flood_fill(std::uint8_t* rgba, int width, int height, int stride, int x, int y,
                Color replacement, int tolerance, Rect* dirty);

}  // namespace brushpad

#endif
