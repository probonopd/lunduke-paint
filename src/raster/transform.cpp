// SPDX-License-Identifier: GPL-3.0-or-later

#include "raster/transform.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace brushpad {
namespace {

Color get(const std::uint8_t* rgba, int stride, int x, int y) {
  const std::uint8_t* p =
      rgba + static_cast<std::size_t>(y) * stride + static_cast<std::size_t>(x) * 4;
  return {p[0], p[1], p[2], p[3]};
}

void put(std::uint8_t* rgba, int stride, int x, int y, Color c) {
  std::uint8_t* p = rgba + static_cast<std::size_t>(y) * stride + static_cast<std::size_t>(x) * 4;
  p[0] = c.r;
  p[1] = c.g;
  p[2] = c.b;
  p[3] = c.a;
}

void put_rgba(std::uint8_t* p, Color c) {
  p[0] = c.r;
  p[1] = c.g;
  p[2] = c.b;
  p[3] = c.a;
}

}  // namespace

void flip_h(std::uint8_t* rgba, int width, int height, int stride) {
  if (rgba == nullptr || width < 2 || height < 1) {
    return;
  }
  std::uint8_t tmp[4];
  for (int y = 0; y < height; ++y) {
    std::uint8_t* row = rgba + static_cast<std::size_t>(y) * stride;
    for (int x = 0; x < width / 2; ++x) {
      std::uint8_t* a = row + static_cast<std::size_t>(x) * 4;
      std::uint8_t* b = row + static_cast<std::size_t>(width - 1 - x) * 4;
      std::memcpy(tmp, a, 4);
      std::memcpy(a, b, 4);
      std::memcpy(b, tmp, 4);
    }
  }
}

void flip_v(std::uint8_t* rgba, int width, int height, int stride) {
  if (rgba == nullptr || width < 1 || height < 2) {
    return;
  }
  std::vector<std::uint8_t> tmp(static_cast<std::size_t>(width) * 4);
  for (int y = 0; y < height / 2; ++y) {
    std::uint8_t* a = rgba + static_cast<std::size_t>(y) * stride;
    std::uint8_t* b = rgba + static_cast<std::size_t>(height - 1 - y) * stride;
    const std::size_t n = static_cast<std::size_t>(width) * 4;
    std::memcpy(tmp.data(), a, n);
    std::memcpy(a, b, n);
    std::memcpy(b, tmp.data(), n);
  }
}

void rotate_180(std::uint8_t* rgba, int width, int height, int stride) {
  flip_h(rgba, width, height, stride);
  flip_v(rgba, width, height, stride);
}

void rotate_90_cw(const std::uint8_t* src, int src_w, int src_h, int src_stride, std::uint8_t* dest,
                  int dest_stride) {
  if (src == nullptr || dest == nullptr) {
    return;
  }
  for (int y = 0; y < src_w; ++y) {
    for (int x = 0; x < src_h; ++x) {
      put(dest, dest_stride, x, y, get(src, src_stride, y, src_h - 1 - x));
    }
  }
}

void rotate_90_ccw(const std::uint8_t* src, int src_w, int src_h, int src_stride, std::uint8_t* dest,
                   int dest_stride) {
  if (src == nullptr || dest == nullptr) {
    return;
  }
  for (int y = 0; y < src_w; ++y) {
    for (int x = 0; x < src_h; ++x) {
      put(dest, dest_stride, x, y, get(src, src_stride, src_w - 1 - y, x));
    }
  }
}

void scale_nearest(const std::uint8_t* src, int src_w, int src_h, int src_stride, std::uint8_t* dest,
                   int dest_w, int dest_h, int dest_stride) {
  if (src == nullptr || dest == nullptr || src_w < 1 || src_h < 1 || dest_w < 1 || dest_h < 1) {
    return;
  }
  for (int y = 0; y < dest_h; ++y) {
    const int sy = std::min(src_h - 1, y * src_h / dest_h);
    std::uint8_t* drow = dest + static_cast<std::size_t>(y) * dest_stride;
    for (int x = 0; x < dest_w; ++x) {
      const int sx = std::min(src_w - 1, x * src_w / dest_w);
      const Color c = get(src, src_stride, sx, sy);
      put_rgba(drow + static_cast<std::size_t>(x) * 4, c);
    }
  }
}

void scale_bilinear(const std::uint8_t* src, int src_w, int src_h, int src_stride, std::uint8_t* dest,
                    int dest_w, int dest_h, int dest_stride) {
  if (src == nullptr || dest == nullptr || src_w < 1 || src_h < 1 || dest_w < 1 || dest_h < 1) {
    return;
  }
  for (int y = 0; y < dest_h; ++y) {
    const double fy = (static_cast<double>(y) + 0.5) * src_h / dest_h - 0.5;
    const int y0 = std::clamp(static_cast<int>(std::floor(fy)), 0, src_h - 1);
    const int y1 = std::min(src_h - 1, y0 + 1);
    const double ty = fy - static_cast<double>(y0);
    std::uint8_t* drow = dest + static_cast<std::size_t>(y) * dest_stride;
    for (int x = 0; x < dest_w; ++x) {
      const double fx = (static_cast<double>(x) + 0.5) * src_w / dest_w - 0.5;
      const int x0 = std::clamp(static_cast<int>(std::floor(fx)), 0, src_w - 1);
      const int x1 = std::min(src_w - 1, x0 + 1);
      const double tx = fx - static_cast<double>(x0);
      const Color c00 = get(src, src_stride, x0, y0);
      const Color c10 = get(src, src_stride, x1, y0);
      const Color c01 = get(src, src_stride, x0, y1);
      const Color c11 = get(src, src_stride, x1, y1);
      auto mix = [&](int ch00, int ch10, int ch01, int ch11) {
        const double a = ch00 * (1.0 - tx) + ch10 * tx;
        const double b = ch01 * (1.0 - tx) + ch11 * tx;
        const int v = static_cast<int>(a * (1.0 - ty) + b * ty + 0.5);
        return static_cast<std::uint8_t>(std::clamp(v, 0, 255));
      };
      Color out;
      out.r = mix(c00.r, c10.r, c01.r, c11.r);
      out.g = mix(c00.g, c10.g, c01.g, c11.g);
      out.b = mix(c00.b, c10.b, c01.b, c11.b);
      out.a = mix(c00.a, c10.a, c01.a, c11.a);
      put_rgba(drow + static_cast<std::size_t>(x) * 4, out);
    }
  }
}

void resize_canvas(const std::uint8_t* src, int src_w, int src_h, int src_stride, std::uint8_t* dest,
                   int dest_w, int dest_h, int dest_stride, Color fill) {
  if (dest == nullptr || dest_w < 1 || dest_h < 1) {
    return;
  }
  for (int y = 0; y < dest_h; ++y) {
    std::uint8_t* drow = dest + static_cast<std::size_t>(y) * dest_stride;
    for (int x = 0; x < dest_w; ++x) {
      if (src != nullptr && x < src_w && y < src_h) {
        put_rgba(drow + static_cast<std::size_t>(x) * 4, get(src, src_stride, x, y));
      } else {
        put_rgba(drow + static_cast<std::size_t>(x) * 4, fill);
      }
    }
  }
}

void crop_rect(const std::uint8_t* src, int src_w, int src_h, int src_stride, Rect rect,
               std::uint8_t* dest, int dest_stride) {
  rect = rect_intersect(rect, Rect{0, 0, src_w, src_h});
  if (src == nullptr || dest == nullptr || rect.empty()) {
    return;
  }
  for (int y = 0; y < rect.h; ++y) {
    const std::uint8_t* s =
        src + static_cast<std::size_t>(rect.y + y) * src_stride + static_cast<std::size_t>(rect.x) * 4;
    std::uint8_t* d = dest + static_cast<std::size_t>(y) * dest_stride;
    std::memcpy(d, s, static_cast<std::size_t>(rect.w) * 4);
  }
}

Rect autocrop_bounds(const std::uint8_t* rgba, int width, int height, int stride) {
  if (rgba == nullptr || width < 1 || height < 1) {
    return {};
  }
  auto row_uniform = [&](int y, Color c) {
    for (int x = 0; x < width; ++x) {
      if (get(rgba, stride, x, y) != c) {
        return false;
      }
    }
    return true;
  };
  auto col_uniform = [&](int x, Color c) {
    for (int y = 0; y < height; ++y) {
      if (get(rgba, stride, x, y) != c) {
        return false;
      }
    }
    return true;
  };
  int top = 0;
  while (top < height - 1 && row_uniform(top, get(rgba, stride, 0, top))) {
    ++top;
  }
  int bottom = height - 1;
  while (bottom > top && row_uniform(bottom, get(rgba, stride, 0, bottom))) {
    --bottom;
  }
  int left = 0;
  while (left < width - 1 && col_uniform(left, get(rgba, stride, left, top))) {
    ++left;
  }
  int right = width - 1;
  while (right > left && col_uniform(right, get(rgba, stride, right, top))) {
    --right;
  }
  return {left, top, right - left + 1, bottom - top + 1};
}

}  // namespace brushpad
