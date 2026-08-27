// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_UI_DIALOGS_NEW_HPP
#define BRUSHPAD_UI_DIALOGS_NEW_HPP

#include <gtkmm/dialog.h>
#include <gtkmm/window.h>

namespace brushpad {

// Phase 0 stub. New currently resets the canvas without a dialog.
class NewImageDialog : public Gtk::Dialog {
public:
  explicit NewImageDialog(Gtk::Window& parent);
};

}  // namespace brushpad

#endif
