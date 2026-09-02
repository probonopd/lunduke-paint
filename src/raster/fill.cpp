// SPDX-License-Identifier: GPL-3.0-or-later

#include "raster/fill.hpp"

#include <algorithm>
#include <vector>

namespace brushpad {
namespace {

Color get_pixel(const std::uint8_t* rgba, int stride, int x, int y) {
  const std::uint8_t* p = rgba + static_cast<std::size_t>(y) * stride + static_cast<std::size_t>(x) * 4;
  return {p[0], p[1], p[2], p[3]};
}

void set_pixel(std::uint8_t* rgba, int stride, int x, int y, Color c) {
  std::uint8_t* p = rgba + static_cast<std::size_t>(y) * stride + static_cast<std::size_t>(x) * 4;
  p[0] = c.r;
  p[1] = c.g;
  p[2] = c.b;
  p[3] = c.a;
}

}  // namespace

void flood_fill(std::uint8_t* rgba, int width, int height, int stride, int x, int y,
                Color replacement, int tolerance, Rect* dirty) {
  if (dirty != nullptr) {
    *dirty = {};
  }
  if (rgba == nullptr || width < 1 || height < 1 || stride < width * 4) {
    return;
  }
  if (x < 0 || y < 0 || x >= width || y >= height) {
    return;
  }
  if (tolerance < 0) {
    tolerance = 0;
  }

  const Color seed = get_pixel(rgba, stride, x, y);
  if (color_chebyshev(seed, replacement) <= 0) {
    return;
  }

  std::vector<std::uint8_t> seen(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0);
  std::vector<std::pair<int, int>> stack;
  stack.reserve(static_cast<std::size_t>(width + height));
  stack.emplace_back(x, y);

  int minx = x;
  int miny = y;
  int maxx = x;
  int maxy = y;

  while (!stack.empty()) {
    const int cx = stack.back().first;
    const int cy = stack.back().second;
    stack.pop_back();
    if (cx < 0 || cy < 0 || cx >= width || cy >= height) {
      continue;
    }
    const std::size_t idx = static_cast<std::size_t>(cy) * static_cast<std::size_t>(width) +
                            static_cast<std::size_t>(cx);
    if (seen[idx] != 0) {
      continue;
    }
    const Color current = get_pixel(rgba, stride, cx, cy);
    if (color_chebyshev(current, seed) > tolerance) {
      continue;
    }
    seen[idx] = 1;
    set_pixel(rgba, stride, cx, cy, replacement);
    if (cx < minx) {
      minx = cx;
    }
    if (cy < miny) {
      miny = cy;
    }
    if (cx > maxx) {
      maxx = cx;
    }
    if (cy > maxy) {
      maxy = cy;
    }
    stack.emplace_back(cx + 1, cy);
    stack.emplace_back(cx - 1, cy);
    stack.emplace_back(cx, cy + 1);
    stack.emplace_back(cx, cy - 1);
  }

  if (dirty != nullptr) {
    *dirty = {minx, miny, maxx - minx + 1, maxy - miny + 1};
  }
}

void flood_mask(const std::uint8_t* rgba, int width, int height, int stride, int x, int y,
                int tolerance, std::vector<std::uint8_t>& mask, Rect* bounds) {
  mask.assign(static_cast<std::size_t>(std::max(0, width)) * static_cast<std::size_t>(std::max(0, height)),
              0);
  if (bounds != nullptr) {
    *bounds = {};
  }
  if (rgba == nullptr || width < 1 || height < 1 || stride < width * 4) {
    return;
  }
  if (x < 0 || y < 0 || x >= width || y >= height) {
    return;
  }
  if (tolerance < 0) {
    tolerance = 0;
  }
  const Color seed = get_pixel(rgba, stride, x, y);
  std::vector<std::uint8_t> seen(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0);
  std::vector<std::pair<int, int>> stack;
  stack.emplace_back(x, y);
  int minx = x;
  int miny = y;
  int maxx = x;
  int maxy = y;
  bool any = false;
  while (!stack.empty()) {
    const int cx = stack.back().first;
    const int cy = stack.back().second;
    stack.pop_back();
    if (cx < 0 || cy < 0 || cx >= width || cy >= height) {
      continue;
    }
    const std::size_t idx = static_cast<std::size_t>(cy) * static_cast<std::size_t>(width) +
                            static_cast<std::size_t>(cx);
    if (seen[idx] != 0) {
      continue;
    }
    const Color current = get_pixel(rgba, stride, cx, cy);
    if (color_chebyshev(current, seed) > tolerance) {
      continue;
    }
    seen[idx] = 1;
    mask[idx] = 255;
    any = true;
    if (cx < minx) minx = cx;
    if (cy < miny) miny = cy;
    if (cx > maxx) maxx = cx;
    if (cy > maxy) maxy = cy;
    stack.emplace_back(cx + 1, cy);
    stack.emplace_back(cx - 1, cy);
    stack.emplace_back(cx, cy + 1);
    stack.emplace_back(cx, cy - 1);
  }
  if (any && bounds != nullptr) {
    *bounds = {minx, miny, maxx - minx + 1, maxy - miny + 1};
  }
}

}  // namespace brushpad
