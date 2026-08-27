// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/colors_panel.hpp"

namespace brushpad {

ColorsPanel::ColorsPanel() : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4) {
  set_border_width(8);
}

}  // namespace brushpad
