// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/tool_options_bar.hpp"

namespace brushpad {

ToolOptionsBar::ToolOptionsBar() : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 6) {
  set_border_width(4);
  set_size_request(-1, 28);
  get_style_context()->add_class("toolbar");
}

}  // namespace brushpad
