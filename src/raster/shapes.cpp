// SPDX-License-Identifier: GPL-3.0-or-later

#include "raster/shapes.hpp"

#include "raster/stroke.hpp"

#include <algorithm>
#include <cmath>

namespace brushpad {
namespace {

void fill_rect_color(std::uint8_t* rgba, int width, int height, int stride, Rect rect, Color color,
                     Rect* dirty) {
  rect = rect_intersect(rect, Rect{0, 0, width, height});
  if (rect.empty()) {
    return;
  }
  for (int y = rect.y; y < rect.y2(); ++y) {
    std::uint8_t* row = rgba + static_cast<std::size_t>(y) * stride;
    for (int x = rect.x; x < rect.x2(); ++x) {
      std::uint8_t* p = row + static_cast<std::size_t>(x) * 4;
      p[0] = color.r;
      p[1] = color.g;
      p[2] = color.b;
      p[3] = color.a;
    }
  }
  if (dirty != nullptr) {
    *dirty = rect_union(*dirty, rect);
  }
}

}  // namespace

void constrain_line_45(int x0, int y0, int* x1, int* y1) {
  if (x1 == nullptr || y1 == nullptr) {
    return;
  }
  const int dx = *x1 - x0;
  const int dy = *y1 - y0;
  const int adx = std::abs(dx);
  const int ady = std::abs(dy);
  if (adx == 0 && ady == 0) {
    return;
  }
  if (adx == 0 || (ady > 0 && ady * 2 < adx)) {
    *y1 = y0;
    return;
  }
  if (ady == 0 || (adx > 0 && adx * 2 < ady)) {
    *x1 = x0;
    return;
  }
  const int m = std::max(adx, ady);
  *x1 = x0 + (dx < 0 ? -m : m);
  *y1 = y0 + (dy < 0 ? -m : m);
}

void constrain_square(int x0, int y0, int* x1, int* y1) {
  if (x1 == nullptr || y1 == nullptr) {
    return;
  }
  const int adx = std::abs(*x1 - x0);
  const int ady = std::abs(*y1 - y0);
  const int m = std::max(adx, ady);
  *x1 = x0 + (*x1 < x0 ? -m : m);
  *y1 = y0 + (*y1 < y0 ? -m : m);
}

void draw_line(std::uint8_t* rgba, int width, int height, int stride, int x0, int y0, int x1,
               int y1, int thickness, Color color, bool antialias, Rect* dirty) {
  if (thickness < 1) {
    thickness = 1;
  }
  if (antialias) {
    stroke_brush(rgba, width, height, stride, static_cast<double>(x0) + 0.5,
                 static_cast<double>(y0) + 0.5, static_cast<double>(x1) + 0.5,
                 static_cast<double>(y1) + 0.5, thickness, color, true, dirty);
  } else {
    stroke_pencil(rgba, width, height, stride, x0, y0, x1, y1, thickness, color, dirty);
  }
}

void draw_rectangle(std::uint8_t* rgba, int width, int height, int stride, int x0, int y0, int x1,
                    int y1, int thickness, Color color, ShapeFillMode mode, bool antialias,
                    Rect* dirty) {
  if (thickness < 1) {
    thickness = 1;
  }
  const int left = std::min(x0, x1);
  const int top = std::min(y0, y1);
  const int right = std::max(x0, x1);
  const int bottom = std::max(y0, y1);
  if (mode == ShapeFillMode::Fill || mode == ShapeFillMode::Both) {
    fill_rect_color(rgba, width, height, stride, Rect{left, top, right - left + 1, bottom - top + 1},
                    color, dirty);
  }
  if (mode == ShapeFillMode::Stroke || mode == ShapeFillMode::Both) {
    draw_line(rgba, width, height, stride, left, top, right, top, thickness, color, antialias,
              dirty);
    draw_line(rgba, width, height, stride, right, top, right, bottom, thickness, color, antialias,
              dirty);
    draw_line(rgba, width, height, stride, right, bottom, left, bottom, thickness, color, antialias,
              dirty);
    draw_line(rgba, width, height, stride, left, bottom, left, top, thickness, color, antialias,
              dirty);
  }
}

void draw_ellipse(std::uint8_t* rgba, int width, int height, int stride, int x0, int y0, int x1,
                  int y1, int thickness, Color color, ShapeFillMode mode, bool antialias,
                  Rect* dirty) {
  if (rgba == nullptr) {
    return;
  }
  if (thickness < 1) {
    thickness = 1;
  }
  const int left = std::min(x0, x1);
  const int top = std::min(y0, y1);
  const int right = std::max(x0, x1);
  const int bottom = std::max(y0, y1);
  const double cx = (static_cast<double>(left) + static_cast<double>(right)) * 0.5 + 0.5;
  const double cy = (static_cast<double>(top) + static_cast<double>(bottom)) * 0.5 + 0.5;
  const double rx = std::max(0.5, (static_cast<double>(right - left) + 1.0) * 0.5);
  const double ry = std::max(0.5, (static_cast<double>(bottom - top) + 1.0) * 0.5);
  const int pad = thickness + 1;
  const int ix0 = std::max(0, left - pad);
  const int iy0 = std::max(0, top - pad);
  const int ix1 = std::min(width, right + pad + 1);
  const int iy1 = std::min(height, bottom + pad + 1);
  if (ix0 >= ix1 || iy0 >= iy1) {
    return;
  }

  const double inv_rx = 1.0 / rx;
  const double inv_ry = 1.0 / ry;
  const double thick = static_cast<double>(thickness);
  const bool do_fill = (mode == ShapeFillMode::Fill || mode == ShapeFillMode::Both);
  const bool do_stroke = (mode == ShapeFillMode::Stroke || mode == ShapeFillMode::Both);

  for (int y = iy0; y < iy1; ++y) {
    std::uint8_t* row = rgba + static_cast<std::size_t>(y) * stride;
    const double dy = (static_cast<double>(y) + 0.5) - cy;
    for (int x = ix0; x < ix1; ++x) {
      const double dx = (static_cast<double>(x) + 0.5) - cx;
      const double nx = dx * inv_rx;
      const double ny = dy * inv_ry;
      const double rnorm = std::sqrt(nx * nx + ny * ny);
      bool hit = false;
      if (do_fill && rnorm <= 1.0) {
        hit = true;
      }
      if (do_stroke) {
        const double along = std::hypot(dx / rx, dy / ry);
        const double dist_px = std::abs(along - 1.0) * std::min(rx, ry);
        if (dist_px <= thick * 0.5 + (antialias ? 0.5 : 0.0)) {
          hit = true;
        }
      }
      if (!hit) {
        continue;
      }
      std::uint8_t* p = row + static_cast<std::size_t>(x) * 4;
      p[0] = color.r;
      p[1] = color.g;
      p[2] = color.b;
      p[3] = color.a;
    }
  }
  if (dirty != nullptr) {
    *dirty = rect_union(*dirty, Rect{ix0, iy0, ix1 - ix0, iy1 - iy0});
  }
}

void draw_rounded_rect(std::uint8_t* rgba, int width, int height, int stride, int x0, int y0,
                       int x1, int y1, int thickness, int radius, Color color, ShapeFillMode mode,
                       bool antialias, Rect* dirty) {
  if (rgba == nullptr) {
    return;
  }
  if (thickness < 1) {
    thickness = 1;
  }
  if (radius < 0) {
    radius = 0;
  }
  const int left = std::min(x0, x1);
  const int top = std::min(y0, y1);
  const int right = std::max(x0, x1);
  const int bottom = std::max(y0, y1);
  const double hw = std::max(0.5, (static_cast<double>(right - left) + 1.0) * 0.5);
  const double hh = std::max(0.5, (static_cast<double>(bottom - top) + 1.0) * 0.5);
  const double cx = (static_cast<double>(left) + static_cast<double>(right)) * 0.5 + 0.5;
  const double cy = (static_cast<double>(top) + static_cast<double>(bottom)) * 0.5 + 0.5;
  double r = std::min(static_cast<double>(radius), std::min(hw, hh));
  const int pad = thickness + 1;
  const int ix0 = std::max(0, left - pad);
  const int iy0 = std::max(0, top - pad);
  const int ix1 = std::min(width, right + pad + 1);
  const int iy1 = std::min(height, bottom + pad + 1);
  if (ix0 >= ix1 || iy0 >= iy1) {
    return;
  }
  const double thick = static_cast<double>(thickness) * 0.5;
  const bool do_fill = (mode == ShapeFillMode::Fill || mode == ShapeFillMode::Both);
  const bool do_stroke = (mode == ShapeFillMode::Stroke || mode == ShapeFillMode::Both);
  const double aa = antialias ? 0.5 : 0.0;
  for (int y = iy0; y < iy1; ++y) {
    std::uint8_t* row = rgba + static_cast<std::size_t>(y) * stride;
    const double py = (static_cast<double>(y) + 0.5) - cy;
    for (int x = ix0; x < ix1; ++x) {
      const double px = (static_cast<double>(x) + 0.5) - cx;
      const double dx = std::abs(px) - (hw - r);
      const double dy = std::abs(py) - (hh - r);
      const double ox = std::max(dx, 0.0);
      const double oy = std::max(dy, 0.0);
      const double sdf = std::sqrt(ox * ox + oy * oy) + std::min(std::max(dx, dy), 0.0) - r;
      bool hit = false;
      if (do_fill && sdf <= 0.0) {
        hit = true;
      }
      if (do_stroke && std::abs(sdf) <= thick + aa) {
        hit = true;
      }
      if (!hit) {
        continue;
      }
      std::uint8_t* p = row + static_cast<std::size_t>(x) * 4;
      p[0] = color.r;
      p[1] = color.g;
      p[2] = color.b;
      p[3] = color.a;
    }
  }
  if (dirty != nullptr) {
    *dirty = rect_union(*dirty, Rect{ix0, iy0, ix1 - ix0, iy1 - iy0});
  }
}

}  // namespace brushpad
