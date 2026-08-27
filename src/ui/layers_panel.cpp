// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/layers_panel.hpp"

namespace brushpad {

LayersPanel::LayersPanel() : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4) {
  set_border_width(8);
}

}  // namespace brushpad
