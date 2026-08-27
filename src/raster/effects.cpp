// SPDX-License-Identifier: GPL-3.0-or-later

#include "raster/effects.hpp"

#include <algorithm>
#include <cmath>

namespace brushpad {
namespace {

int clamp_byte(int v) {
  if (v < 0) {
    return 0;
  }
  if (v > 255) {
    return 255;
  }
  return v;
}

void rgb_to_hsv(std::uint8_t r, std::uint8_t g, std::uint8_t b, float& h, float& s, float& v) {
  const float rf = static_cast<float>(r) / 255.0f;
  const float gf = static_cast<float>(g) / 255.0f;
  const float bf = static_cast<float>(b) / 255.0f;
  const float maxc = std::max(rf, std::max(gf, bf));
  const float minc = std::min(rf, std::min(gf, bf));
  const float d = maxc - minc;
  v = maxc;
  s = (maxc <= 0.0f) ? 0.0f : d / maxc;
  if (d <= 1e-6f) {
    h = 0.0f;
    return;
  }
  if (maxc == rf) {
    h = 60.0f * std::fmod((gf - bf) / d, 6.0f);
  } else if (maxc == gf) {
    h = 60.0f * ((bf - rf) / d + 2.0f);
  } else {
    h = 60.0f * ((rf - gf) / d + 4.0f);
  }
  if (h < 0.0f) {
    h += 360.0f;
  }
}

void hsv_to_rgb(float h, float s, float v, std::uint8_t& r, std::uint8_t& g, std::uint8_t& b) {
  if (s <= 0.0f) {
    const int gray = clamp_byte(static_cast<int>(v * 255.0f + 0.5f));
    r = g = b = static_cast<std::uint8_t>(gray);
    return;
  }
  h = std::fmod(h, 360.0f);
  if (h < 0.0f) {
    h += 360.0f;
  }
  const float c = v * s;
  const float x = c * (1.0f - std::fabs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
  const float m = v - c;
  float rf = 0;
  float gf = 0;
  float bf = 0;
  if (h < 60.0f) {
    rf = c;
    gf = x;
  } else if (h < 120.0f) {
    rf = x;
    gf = c;
  } else if (h < 180.0f) {
    gf = c;
    bf = x;
  } else if (h < 240.0f) {
    gf = x;
    bf = c;
  } else if (h < 300.0f) {
    rf = x;
    bf = c;
  } else {
    rf = c;
    bf = x;
  }
  r = static_cast<std::uint8_t>(clamp_byte(static_cast<int>((rf + m) * 255.0f + 0.5f)));
  g = static_cast<std::uint8_t>(clamp_byte(static_cast<int>((gf + m) * 255.0f + 0.5f)));
  b = static_cast<std::uint8_t>(clamp_byte(static_cast<int>((bf + m) * 255.0f + 0.5f)));
}

}  // namespace

void invert_rgba(std::uint8_t* rgba, int width, int height, int stride) {
  if (rgba == nullptr || width < 1 || height < 1) {
    return;
  }
  for (int y = 0; y < height; ++y) {
    std::uint8_t* row = rgba + static_cast<std::size_t>(y) * static_cast<std::size_t>(stride);
    for (int x = 0; x < width; ++x) {
      std::uint8_t* p = row + static_cast<std::size_t>(x) * 4;
      p[0] = static_cast<std::uint8_t>(255 - p[0]);
      p[1] = static_cast<std::uint8_t>(255 - p[1]);
      p[2] = static_cast<std::uint8_t>(255 - p[2]);
    }
  }
}

void grayscale_rgba(std::uint8_t* rgba, int width, int height, int stride) {
  if (rgba == nullptr || width < 1 || height < 1) {
    return;
  }
  for (int y = 0; y < height; ++y) {
    std::uint8_t* row = rgba + static_cast<std::size_t>(y) * static_cast<std::size_t>(stride);
    for (int x = 0; x < width; ++x) {
      std::uint8_t* p = row + static_cast<std::size_t>(x) * 4;
      const int gray = (static_cast<int>(p[0]) * 77 + static_cast<int>(p[1]) * 150 +
                        static_cast<int>(p[2]) * 29) >>
                       8;
      p[0] = p[1] = p[2] = static_cast<std::uint8_t>(gray);
    }
  }
}

void brightness_contrast_rgba(std::uint8_t* rgba, int width, int height, int stride,
                              int brightness, int contrast) {
  if (rgba == nullptr || width < 1 || height < 1) {
    return;
  }
  brightness = std::clamp(brightness, -100, 100);
  contrast = std::clamp(contrast, -100, 100);
  const int add = brightness * 255 / 100;
  const float factor = (100.0f + static_cast<float>(contrast)) / 100.0f;
  for (int y = 0; y < height; ++y) {
    std::uint8_t* row = rgba + static_cast<std::size_t>(y) * static_cast<std::size_t>(stride);
    for (int x = 0; x < width; ++x) {
      std::uint8_t* p = row + static_cast<std::size_t>(x) * 4;
      for (int c = 0; c < 3; ++c) {
        const float v = (static_cast<float>(p[c]) - 128.0f) * factor + 128.0f + static_cast<float>(add);
        p[c] = static_cast<std::uint8_t>(clamp_byte(static_cast<int>(v + (v >= 0.0f ? 0.5f : -0.5f))));
      }
    }
  }
}

void hue_saturation_rgba(std::uint8_t* rgba, int width, int height, int stride, int hue,
                         int saturation) {
  if (rgba == nullptr || width < 1 || height < 1) {
    return;
  }
  hue = std::clamp(hue, -180, 180);
  saturation = std::clamp(saturation, -100, 100);
  const float dh = static_cast<float>(hue);
  const float ds = static_cast<float>(saturation) / 100.0f;
  for (int y = 0; y < height; ++y) {
    std::uint8_t* row = rgba + static_cast<std::size_t>(y) * static_cast<std::size_t>(stride);
    for (int x = 0; x < width; ++x) {
      std::uint8_t* p = row + static_cast<std::size_t>(x) * 4;
      float h = 0;
      float s = 0;
      float v = 0;
      rgb_to_hsv(p[0], p[1], p[2], h, s, v);
      h += dh;
      s = std::clamp(s + ds, 0.0f, 1.0f);
      hsv_to_rgb(h, s, v, p[0], p[1], p[2]);
    }
  }
}

void posterize_rgba(std::uint8_t* rgba, int width, int height, int stride, int levels) {
  if (rgba == nullptr || width < 1 || height < 1) {
    return;
  }
  levels = std::clamp(levels, 2, 16);
  const int denom = levels - 1;
  for (int y = 0; y < height; ++y) {
    std::uint8_t* row = rgba + static_cast<std::size_t>(y) * static_cast<std::size_t>(stride);
    for (int x = 0; x < width; ++x) {
      std::uint8_t* p = row + static_cast<std::size_t>(x) * 4;
      for (int c = 0; c < 3; ++c) {
        const int q = (static_cast<int>(p[c]) * denom + 127) / 255;
        p[c] = static_cast<std::uint8_t>((q * 255) / denom);
      }
    }
  }
}

}  // namespace brushpad
