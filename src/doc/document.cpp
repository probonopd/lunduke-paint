// SPDX-License-Identifier: GPL-3.0-or-later

#include "doc/document.hpp"

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

}  // namespace brushpad
