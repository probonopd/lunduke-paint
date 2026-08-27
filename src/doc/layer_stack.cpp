// SPDX-License-Identifier: GPL-3.0-or-later

#include "doc/layer_stack.hpp"

#include <stdexcept>

namespace brushpad {

void LayerStack::reset(int width, int height, Color fill, const std::string& layer_name) {
  width_ = width;
  height_ = height;
  layers_.clear();
  layers_.push_back(std::make_unique<Layer>(width, height, fill, layer_name));
  tool_layer_ = std::make_unique<Layer>(width, height, Color::transparent(), "Tool");
  active_ = 0;
}

void LayerStack::set_active_index(int index) {
  if (index >= 0 && index < count()) {
    active_ = index;
  }
}

Layer& LayerStack::active_layer() {
  return at(active_);
}

const Layer& LayerStack::active_layer() const {
  return at(active_);
}

Layer& LayerStack::at(int index) {
  if (index < 0 || index >= count()) {
    throw std::out_of_range("layer index");
  }
  return *layers_[static_cast<std::size_t>(index)];
}

const Layer& LayerStack::at(int index) const {
  if (index < 0 || index >= count()) {
    throw std::out_of_range("layer index");
  }
  return *layers_[static_cast<std::size_t>(index)];
}

Layer& LayerStack::tool_layer() {
  if (!tool_layer_) {
    tool_layer_ = std::make_unique<Layer>(width_, height_, Color::transparent(), "Tool");
  }
  return *tool_layer_;
}

const Layer& LayerStack::tool_layer() const {
  return *tool_layer_;
}

void LayerStack::clear_tool_layer() {
  if (tool_layer_) {
    tool_layer_->clear_transparent();
  }
}

void LayerStack::copy_active_to_tool() {
  tool_layer().copy_from(active_layer());
}

}  // namespace brushpad
