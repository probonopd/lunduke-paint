// SPDX-License-Identifier: GPL-3.0-or-later

#include "doc/layer.hpp"

#include <algorithm>
#include <cstring>

namespace brushpad {

Layer::Layer(int width, int height, Color fill, std::string name)
    : name_(std::move(name)), width_(width), height_(height), stride_(width * 4) {
  if (width_ < 1) {
    width_ = 1;
  }
  if (height_ < 1) {
    height_ = 1;
  }
  stride_ = width_ * 4;
  pixels_.assign(static_cast<std::size_t>(stride_) * static_cast<std::size_t>(height_), 0);
  this->fill(fill);
}

Color Layer::pixel(int x, int y) const {
  if (x < 0 || y < 0 || x >= width_ || y >= height_) {
    return Color::transparent();
  }
  const std::uint8_t* p = pixels_.data() + static_cast<std::size_t>(y) * stride_ +
                          static_cast<std::size_t>(x) * 4;
  return {p[0], p[1], p[2], p[3]};
}

void Layer::set_pixel(int x, int y, Color color) {
  if (x < 0 || y < 0 || x >= width_ || y >= height_) {
    return;
  }
  std::uint8_t* p = pixels_.data() + static_cast<std::size_t>(y) * stride_ +
                    static_cast<std::size_t>(x) * 4;
  p[0] = color.r;
  p[1] = color.g;
  p[2] = color.b;
  p[3] = color.a;
}

void Layer::fill(Color color) {
  for (int y = 0; y < height_; ++y) {
    std::uint8_t* row = pixels_.data() + static_cast<std::size_t>(y) * stride_;
    for (int x = 0; x < width_; ++x) {
      std::uint8_t* p = row + static_cast<std::size_t>(x) * 4;
      p[0] = color.r;
      p[1] = color.g;
      p[2] = color.b;
      p[3] = color.a;
    }
  }
}

void Layer::clear_transparent() {
  std::fill(pixels_.begin(), pixels_.end(), static_cast<std::uint8_t>(0));
}


void Layer::fill_rect(Rect rect, Color color) {
  rect = rect_intersect(rect, Rect{0, 0, width_, height_});
  if (rect.empty()) {
    return;
  }
  for (int y = rect.y; y < rect.y2(); ++y) {
    std::uint8_t* row = pixels_.data() + static_cast<std::size_t>(y) * stride_;
    for (int x = rect.x; x < rect.x2(); ++x) {
      std::uint8_t* p = row + static_cast<std::size_t>(x) * 4;
      p[0] = color.r;
      p[1] = color.g;
      p[2] = color.b;
      p[3] = color.a;
    }
  }
}

void Layer::set_pixels(int width, int height, const std::uint8_t* rgba, int stride) {
  if (width < 1) {
    width = 1;
  }
  if (height < 1) {
    height = 1;
  }
  width_ = width;
  height_ = height;
  stride_ = width_ * 4;
  pixels_.assign(static_cast<std::size_t>(stride_) * static_cast<std::size_t>(height_), 0);
  if (rgba == nullptr) {
    return;
  }
  const int row_bytes = width_ * 4;
  for (int y = 0; y < height_; ++y) {
    std::memcpy(pixels_.data() + static_cast<std::size_t>(y) * stride_,
                rgba + static_cast<std::size_t>(y) * static_cast<std::size_t>(stride),
                static_cast<std::size_t>(row_bytes));
  }
}

void Layer::copy_rect_from(const Layer& src, Rect rect) {
  rect = rect_intersect(rect, Rect{0, 0, width_, height_});
  rect = rect_intersect(rect, Rect{0, 0, src.width_, src.height_});
  if (rect.empty()) {
    return;
  }
  for (int y = 0; y < rect.h; ++y) {
    const std::uint8_t* s = src.pixels_.data() +
                            static_cast<std::size_t>(rect.y + y) * src.stride_ +
                            static_cast<std::size_t>(rect.x) * 4;
    std::uint8_t* d = pixels_.data() + static_cast<std::size_t>(rect.y + y) * stride_ +
                      static_cast<std::size_t>(rect.x) * 4;
    std::memcpy(d, s, static_cast<std::size_t>(rect.w) * 4);
  }
}

void Layer::write_rect(Rect rect, const std::uint8_t* rgba) {
  rect = rect_intersect(rect, Rect{0, 0, width_, height_});
  if (rect.empty() || rgba == nullptr) {
    return;
  }
  for (int y = 0; y < rect.h; ++y) {
    std::uint8_t* d = pixels_.data() + static_cast<std::size_t>(rect.y + y) * stride_ +
                      static_cast<std::size_t>(rect.x) * 4;
    const std::uint8_t* s = rgba + static_cast<std::size_t>(y) * static_cast<std::size_t>(rect.w) * 4;
    std::memcpy(d, s, static_cast<std::size_t>(rect.w) * 4);
  }
}

void Layer::read_rect(Rect rect, std::uint8_t* rgba) const {
  rect = rect_intersect(rect, Rect{0, 0, width_, height_});
  if (rect.empty() || rgba == nullptr) {
    return;
  }
  for (int y = 0; y < rect.h; ++y) {
    const std::uint8_t* s = pixels_.data() + static_cast<std::size_t>(rect.y + y) * stride_ +
                            static_cast<std::size_t>(rect.x) * 4;
    std::uint8_t* d = rgba + static_cast<std::size_t>(y) * static_cast<std::size_t>(rect.w) * 4;
    std::memcpy(d, s, static_cast<std::size_t>(rect.w) * 4);
  }
}

void Layer::copy_from(const Layer& src) {
  if (src.width_ != width_ || src.height_ != height_) {
    return;
  }
  pixels_ = src.pixels_;
}

}  // namespace brushpad
