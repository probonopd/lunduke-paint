// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/status_bar.hpp"

#include <cstdio>

namespace brushpad {

StatusBar::StatusBar() {
  context_id_ = get_context_id("brushpad");
  show_message("Ready");
}

void StatusBar::show_coordinates(double x, double y) {
  char buffer[64];
  std::snprintf(buffer, sizeof(buffer), "Canvas: %.0f, %.0f", x, y);
  pop(context_id_);
  push(buffer, context_id_);
}

void StatusBar::clear_coordinates() {
  pop(context_id_);
  push("Ready", context_id_);
}

void StatusBar::show_message(const Glib::ustring& message) {
  pop(context_id_);
  push(message, context_id_);
}

}  // namespace brushpad
