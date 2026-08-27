// SPDX-License-Identifier: GPL-3.0-or-later

#include "raster/text.hpp"

#include <algorithm>

namespace brushpad {

void blit_rgba_buffer(std::uint8_t* dest, int dw, int dh, int dstride, int dx, int dy,
                      const std::uint8_t* src, int sw, int sh, int sstride, bool skip_transparent,
                      Rect* dirty) {
  if (dest == nullptr || src == nullptr || sw < 1 || sh < 1 || dw < 1 || dh < 1) {
    return;
  }
  int minx = dw;
  int miny = dh;
  int maxx = -1;
  int maxy = -1;
  for (int y = 0; y < sh; ++y) {
    const int dyi = dy + y;
    if (dyi < 0 || dyi >= dh) {
      continue;
    }
    const std::uint8_t* srow = src + static_cast<std::size_t>(y) * static_cast<std::size_t>(sstride);
    std::uint8_t* drow = dest + static_cast<std::size_t>(dyi) * static_cast<std::size_t>(dstride);
    for (int x = 0; x < sw; ++x) {
      const int dxi = dx + x;
      if (dxi < 0 || dxi >= dw) {
        continue;
      }
      const std::uint8_t* s = srow + static_cast<std::size_t>(x) * 4;
      if (skip_transparent && s[3] == 0) {
        continue;
      }
      std::uint8_t* d = drow + static_cast<std::size_t>(dxi) * 4;
      d[0] = s[0];
      d[1] = s[1];
      d[2] = s[2];
      d[3] = s[3];
      if (dxi < minx) {
        minx = dxi;
      }
      if (dyi < miny) {
        miny = dyi;
      }
      if (dxi > maxx) {
        maxx = dxi;
      }
      if (dyi > maxy) {
        maxy = dyi;
      }
    }
  }
  if (dirty != nullptr && maxx >= minx) {
    *dirty = rect_union(*dirty, Rect{minx, miny, maxx - minx + 1, maxy - miny + 1});
  }
}

}  // namespace brushpad
