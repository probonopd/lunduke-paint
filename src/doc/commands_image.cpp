// SPDX-License-Identifier: GPL-3.0-or-later

#include "doc/commands_image.hpp"

#include "doc/document.hpp"
#include "doc/layer.hpp"

#include <algorithm>
#include <cstring>

namespace lundukepaint {

std::vector<std::uint8_t> copy_layer_pixels(const Layer& layer) {
  std::vector<std::uint8_t> out(static_cast<std::size_t>(layer.width()) *
                                static_cast<std::size_t>(layer.height()) * 4);
  if (layer.stride() == layer.width() * 4) {
    std::memcpy(out.data(), layer.pixels(), out.size());
    return out;
  }
  for (int y = 0; y < layer.height(); ++y) {
    std::memcpy(out.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(layer.width()) * 4,
                layer.pixels() + static_cast<std::size_t>(y) * layer.stride(),
                static_cast<std::size_t>(layer.width()) * 4);
  }
  return out;
}

std::unique_ptr<LayerBufferCommand> LayerBufferCommand::from_buffers(
    std::string name, int old_w, int old_h, const std::uint8_t* old_px, int old_stride, int new_w,
    int new_h, const std::uint8_t* new_px, int new_stride, int layer_index) {
  auto cmd = std::unique_ptr<LayerBufferCommand>(new LayerBufferCommand());
  cmd->name_ = std::move(name);
  cmd->layer_index_ = layer_index;
  cmd->old_w_ = old_w;
  cmd->old_h_ = old_h;
  cmd->new_w_ = new_w;
  cmd->new_h_ = new_h;
  cmd->old_px_.assign(static_cast<std::size_t>(old_w) * static_cast<std::size_t>(old_h) * 4, 0);
  cmd->new_px_.assign(static_cast<std::size_t>(new_w) * static_cast<std::size_t>(new_h) * 4, 0);
  if (old_px != nullptr) {
    for (int y = 0; y < old_h; ++y) {
      std::memcpy(cmd->old_px_.data() +
                      static_cast<std::size_t>(y) * static_cast<std::size_t>(old_w) * 4,
                  old_px + static_cast<std::size_t>(y) * static_cast<std::size_t>(old_stride),
                  static_cast<std::size_t>(old_w) * 4);
    }
  }
  if (new_px != nullptr) {
    for (int y = 0; y < new_h; ++y) {
      std::memcpy(cmd->new_px_.data() +
                      static_cast<std::size_t>(y) * static_cast<std::size_t>(new_w) * 4,
                  new_px + static_cast<std::size_t>(y) * static_cast<std::size_t>(new_stride),
                  static_cast<std::size_t>(new_w) * 4);
    }
  }
  return cmd;
}

void LayerBufferCommand::apply(Document& document) {
  document.replace_active_buffer(new_w_, new_h_, new_px_.data(), new_w_ * 4);
}

void LayerBufferCommand::undo(Document& document) {
  document.replace_active_buffer(old_w_, old_h_, old_px_.data(), old_w_ * 4);
}

Rect LayerBufferCommand::dirty_rect() const {
  return {0, 0, std::max(old_w_, new_w_), std::max(old_h_, new_h_)};
}

}  // namespace lundukepaint
