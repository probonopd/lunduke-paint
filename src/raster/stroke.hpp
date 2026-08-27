// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_RASTER_STROKE_HPP
#define BRUSHPAD_RASTER_STROKE_HPP

#include "raster/types.hpp"

#include <cstdint>

namespace brushpad {

// Hard square stamp, no anti-alias. size is N×N pixels.
void stamp_square(std::uint8_t* rgba, int width, int height, int stride, int cx, int cy,
                  int size, Color color, Rect* dirty);

// Round brush stamp. If antialias is true, coverage is blended (straight alpha).
void stamp_round(std::uint8_t* rgba, int width, int height, int stride, double cx, double cy,
                 int size, Color color, bool antialias, Rect* dirty);

// Bresenham / sampled line of square stamps (pencil).
void stroke_pencil(std::uint8_t* rgba, int width, int height, int stride, int x0, int y0, int x1,
                   int y1, int size, Color color, Rect* dirty);

// Sampled line of round stamps (brush / eraser).
void stroke_brush(std::uint8_t* rgba, int width, int height, int stride, double x0, double y0,
                  double x1, double y1, int size, Color color, bool antialias, Rect* dirty);

}  // namespace brushpad

#endif
