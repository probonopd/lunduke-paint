// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/toolbox.hpp"

namespace brushpad {

Toolbox::Toolbox() : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4) {
  set_border_width(4);
  set_size_request(56, -1);
}

}  // namespace brushpad
