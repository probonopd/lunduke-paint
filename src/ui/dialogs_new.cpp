// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/dialogs_new.hpp"

namespace brushpad {

NewImageDialog::NewImageDialog(Gtk::Window& parent)
    : Gtk::Dialog("New Image", parent, true) {
  add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  add_button("_OK", Gtk::RESPONSE_OK);
  set_default_response(Gtk::RESPONSE_OK);
}

}  // namespace brushpad
