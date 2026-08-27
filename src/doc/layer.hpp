// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_DOC_LAYER_HPP
#define BRUSHPAD_DOC_LAYER_HPP

#include "raster/types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace brushpad {

class Layer {
public:
  Layer(int width, int height, Color fill, std::string name);

  const std::string& name() const { return name_; }
  void set_name(std::string name) { name_ = std::move(name); }

  bool visible() const { return visible_; }
  void set_visible(bool v) { visible_ = v; }

  bool locked() const { return locked_; }
  void set_locked(bool v) { locked_ = v; }

  float opacity() const { return opacity_; }
  void set_opacity(float v) { opacity_ = v; }

  int blend() const { return blend_; }
  void set_blend(int v) { blend_ = v; }

  int offset_x() const { return offset_x_; }
  int offset_y() const { return offset_y_; }

  int width() const { return width_; }
  int height() const { return height_; }
  int stride() const { return stride_; }

  std::uint8_t* pixels() { return pixels_.data(); }
  const std::uint8_t* pixels() const { return pixels_.data(); }

  Color pixel(int x, int y) const;
  void set_pixel(int x, int y, Color color);

  void fill(Color color);
  void clear_transparent();

  void copy_rect_from(const Layer& src, Rect rect);
  void write_rect(Rect rect, const std::uint8_t* rgba);
  void read_rect(Rect rect, std::uint8_t* rgba) const;

  void copy_from(const Layer& src);

private:
  std::string name_;
  bool visible_ = true;
  bool locked_ = false;
  float opacity_ = 1.0f;
  int blend_ = 0;
  int offset_x_ = 0;
  int offset_y_ = 0;
  int width_ = 0;
  int height_ = 0;
  int stride_ = 0;
  std::vector<std::uint8_t> pixels_;
};

}  // namespace brushpad

#endif
