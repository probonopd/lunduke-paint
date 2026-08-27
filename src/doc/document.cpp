// SPDX-License-Identifier: GPL-3.0-or-later

#include "doc/document.hpp"

#include "doc/commands_pixels.hpp"
#include "doc/layer.hpp"

#include <utility>

namespace brushpad {

Document::Document(int width, int height, Color background, std::string layer_name)
    : width_(width), height_(height), canvas_bg_(background) {
  layers_.reset(width_, height_, background, std::move(layer_name));
}

std::unique_ptr<Document> Document::create(int width, int height, Color background,
                                           std::string layer_name) {
  return std::unique_ptr<Document>(new Document(width, height, background, std::move(layer_name)));
}

void Document::set_dirty(bool dirty) {
  if (dirty_ == dirty) {
    return;
  }
  dirty_ = dirty;
  notify_changed();
}

void Document::mark_clean() {
  set_dirty(false);
}

void Document::set_foreground(Color color) {
  if (fg_ == color) {
    return;
  }
  fg_ = color;
  notify_changed();
}

void Document::set_background(Color color) {
  if (bg_ == color) {
    return;
  }
  bg_ = color;
  notify_changed();
}

void Document::swap_colors() {
  std::swap(fg_, bg_);
  notify_changed();
}

void Document::reset_colors() {
  fg_ = Color::black();
  bg_ = Color::white();
  notify_changed();
}

void Document::commit(std::unique_ptr<Command> command) {
  if (!command) {
    return;
  }
  const Rect dirty = command->dirty_rect();
  history_.commit(*this, std::move(command));
  dirty_ = true;
  notify_invalidated(dirty);
  notify_changed();
}

Rect Document::undo() {
  if (!history_.can_undo()) {
    return {};
  }
  const Rect dirty = history_.undo(*this);
  dirty_ = true;
  notify_invalidated(dirty);
  notify_changed();
  return dirty;
}

Rect Document::redo() {
  if (!history_.can_redo()) {
    return {};
  }
  const Rect dirty = history_.redo(*this);
  dirty_ = true;
  notify_invalidated(dirty);
  notify_changed();
  return dirty;
}

void Document::notify_invalidated(Rect rect) {
  if (on_invalidated_) {
    on_invalidated_(rect);
  }
}

void Document::notify_changed() {
  if (on_changed_) {
    on_changed_();
  }
}


void Document::select_all() {
  commit_floating();
  selection_.select_all(width_, height_);
  notify_invalidated({0, 0, width_, height_});
  notify_changed();
}

void Document::deselect() {
  commit_floating();
  if (selection_.empty()) {
    return;
  }
  const Rect dirty = selection_.bounds().empty() ? Rect{0, 0, width_, height_}
                                                 : selection_.bounds();
  selection_.clear();
  notify_invalidated(dirty);
  notify_changed();
}

void Document::invert_selection() {
  commit_floating();
  selection_.invert(width_, height_);
  notify_invalidated({0, 0, width_, height_});
  notify_changed();
}

bool Document::commit_floating(const char* name) {
  if (!selection_.floating()) {
    return false;
  }
  Layer& layer = layers_.active_layer();
  Layer before(layer.width(), layer.height(), Color::transparent(), "before");
  before.copy_from(layer);
  Rect dirty = selection_.dirty_union();
  if (!selection_.copy_mode()) {
    layer.fill_rect(selection_.origin_rect(), Color::transparent());
    dirty = rect_union(dirty, selection_.origin_rect());
  }
  blit_rgba(layer, selection_.float_x(), selection_.float_y(), selection_.float_pixels(),
            selection_.float_w(), selection_.float_h(), selection_.float_w() * 4,
            selection_.transparent_move());
  dirty = rect_union(dirty, selection_.float_rect());
  const Rect kept = selection_.float_rect();
  const bool transparent = selection_.transparent_move();
  selection_.drop_float();
  if (!kept.empty()) {
    selection_.set_rect(kept);
    selection_.set_transparent_move(transparent);
  } else {
    selection_.clear();
  }
  auto cmd = PixelPatchCommand::from_layers(before, layer, dirty, name ? name : "Move selection",
                                            layers_.active_index());
  if (cmd && !cmd->empty()) {
    commit(std::move(cmd));
  } else {
    notify_invalidated(dirty);
    notify_changed();
  }
  return true;
}

void Document::delete_selection() {
  if (selection_.empty()) {
    return;
  }
  Layer& layer = layers_.active_layer();
  Layer before(layer.width(), layer.height(), Color::transparent(), "before");
  before.copy_from(layer);
  Rect dirty{};
  if (selection_.floating()) {
    if (!selection_.copy_mode()) {
      layer.fill_rect(selection_.origin_rect(), Color::transparent());
      dirty = selection_.origin_rect();
    }
    const Rect extra = selection_.dirty_union();
    dirty = rect_union(dirty, extra);
    selection_.clear();
  } else {
    fill_selection(layer, selection_, Color::transparent(), &dirty);
  }
  auto cmd = PixelPatchCommand::from_layers(before, layer, dirty, "Delete", layers_.active_index());
  if (cmd && !cmd->empty()) {
    commit(std::move(cmd));
  } else {
    notify_invalidated(dirty);
    notify_changed();
  }
}

void Document::duplicate_selection() {
  if (selection_.empty()) {
    return;
  }
  if (selection_.floating()) {
    commit_floating("Duplicate");
  }
  if (selection_.empty() || selection_.inverted()) {
    return;
  }
  if (!selection_.lift(layers_.active_layer())) {
    return;
  }
  selection_.set_copy_mode(true);
  selection_.move_float(selection_.float_x() + 8, selection_.float_y() + 8);
  notify_invalidated(selection_.dirty_union());
  notify_changed();
}

void Document::paste_floating(int x, int y, int w, int h, std::vector<std::uint8_t> rgba) {
  commit_floating();
  selection_.set_float_pixels(x, y, w, h, std::move(rgba));
  notify_invalidated(selection_.dirty_union());
  notify_changed();
}

void Document::replace_active_buffer(int width, int height, const std::uint8_t* rgba, int stride) {
  if (width < 1) {
    width = 1;
  }
  if (height < 1) {
    height = 1;
  }
  width_ = width;
  height_ = height;
  layers_.replace_active(width_, height_, rgba, stride);
  layers_.resize_scratch(width_, height_);
  selection_.clear();
}

}  // namespace brushpad
