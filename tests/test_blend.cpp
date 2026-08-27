// SPDX-License-Identifier: GPL-3.0-or-later

#include "raster/blend.hpp"

#include <cstdio>

namespace {

using brushpad::BlendMode;
using brushpad::blend_pixel;

int expect(bool cond, const char* msg) {
  if (!cond) {
    std::fprintf(stderr, "test_blend: %s\n", msg);
    return 1;
  }
  return 0;
}

}  // namespace

int main() {
  int errors = 0;

  // Multiply red 255,0,0 opaque over white: result red.
  {
    std::uint8_t dest[4] = {255, 255, 255, 255};
    const std::uint8_t src[4] = {255, 0, 0, 255};
    blend_pixel(dest, src, BlendMode::Multiply, 1.0f);
    errors += expect(dest[0] == 255 && dest[1] == 0 && dest[2] == 0 && dest[3] == 255,
                     "multiply red over white");
  }

  // Multiply 128,128,128 over 128,128,128: ~64,64,64
  {
    std::uint8_t dest[4] = {128, 128, 128, 255};
    const std::uint8_t src[4] = {128, 128, 128, 255};
    blend_pixel(dest, src, BlendMode::Multiply, 1.0f);
    errors += expect(dest[0] >= 63 && dest[0] <= 65, "multiply gray r");
    errors += expect(dest[1] >= 63 && dest[1] <= 65, "multiply gray g");
    errors += expect(dest[2] >= 63 && dest[2] <= 65, "multiply gray b");
    errors += expect(dest[3] == 255, "multiply gray a");
  }

  // Screen black over mid gray stays mid gray; screen white goes white.
  {
    std::uint8_t dest[4] = {128, 128, 128, 255};
    const std::uint8_t src[4] = {255, 255, 255, 255};
    blend_pixel(dest, src, BlendMode::Screen, 1.0f);
    errors += expect(dest[0] == 255 && dest[1] == 255 && dest[2] == 255, "screen white");
  }

  // Hidden-style: zero opacity leaves dest unchanged.
  {
    std::uint8_t dest[4] = {10, 20, 30, 255};
    const std::uint8_t src[4] = {255, 0, 0, 255};
    blend_pixel(dest, src, BlendMode::Normal, 0.0f);
    errors += expect(dest[0] == 10 && dest[1] == 20 && dest[2] == 30 && dest[3] == 255,
                     "zero opacity is a no-op");
  }

  // Transparent dest takes source (even for Multiply).
  {
    std::uint8_t dest[4] = {0, 0, 0, 0};
    const std::uint8_t src[4] = {40, 80, 120, 200};
    blend_pixel(dest, src, BlendMode::Multiply, 1.0f);
    errors += expect(dest[0] == 40 && dest[1] == 80 && dest[2] == 120 && dest[3] == 200,
                     "multiply onto transparent dest");
  }

  if (errors != 0) {
    std::fprintf(stderr, "test_blend: %d failure(s)\n", errors);
    return 1;
  }
  std::printf("test_blend: ok\n");
  return 0;
}
