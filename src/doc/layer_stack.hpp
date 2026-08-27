// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_DOC_LAYER_STACK_HPP
#define BRUSHPAD_DOC_LAYER_STACK_HPP

#include "doc/layer.hpp"

#include <memory>
#include <vector>

namespace brushpad {

class LayerStack {
public:
  LayerStack() = default;

  void reset(int width, int height, Color fill, const std::string& layer_name);

  int count() const { return static_cast<int>(layers_.size()); }
  int active_index() const { return active_; }
  void set_active_index(int index);

  Layer& active_layer();
  const Layer& active_layer() const;

  Layer& at(int index);
  const Layer& at(int index) const;

  Layer& tool_layer();
  const Layer& tool_layer() const;
  void clear_tool_layer();
  void copy_active_to_tool();

private:
  std::vector<std::unique_ptr<Layer>> layers_;
  std::unique_ptr<Layer> tool_layer_;
  int active_ = 0;
  int width_ = 0;
  int height_ = 0;
};

}  // namespace brushpad

#endif
