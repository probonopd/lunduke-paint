// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_RASTER_SHAPES_HPP
#define BRUSHPAD_RASTER_SHAPES_HPP

#include "raster/types.hpp"

#include <cstdint>

namespace brushpad {

enum class ShapeFillMode { Stroke, Fill, Both };

void draw_line(std::uint8_t* rgba, int width, int height, int stride, int x0, int y0, int x1,
               int y1, int thickness, Color color, bool antialias, Rect* dirty);

void draw_rectangle(std::uint8_t* rgba, int width, int height, int stride, int x0, int y0, int x1,
                    int y1, int thickness, Color color, ShapeFillMode mode, bool antialias,
                    Rect* dirty);

void draw_ellipse(std::uint8_t* rgba, int width, int height, int stride, int x0, int y0, int x1,
                  int y1, int thickness, Color color, ShapeFillMode mode, bool antialias,
                  Rect* dirty);

void draw_rounded_rect(std::uint8_t* rgba, int width, int height, int stride, int x0, int y0,
                       int x1, int y1, int thickness, int radius, Color color, ShapeFillMode mode,
                       bool antialias, Rect* dirty);

void constrain_line_45(int x0, int y0, int* x1, int* y1);
void constrain_square(int x0, int y0, int* x1, int* y1);

}  // namespace brushpad

#endif
