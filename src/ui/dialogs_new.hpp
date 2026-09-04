// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef LUNDUKEPAINT_UI_DIALOGS_NEW_HPP
#define LUNDUKEPAINT_UI_DIALOGS_NEW_HPP

#include "raster/types.hpp"

#include <gtkmm/colorbutton.h>
#include <gtkmm/dialog.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/window.h>

namespace lundukepaint {

class NewImageDialog : public Gtk::Dialog {
public:
  explicit NewImageDialog(Gtk::Window& parent, int width = kDefaultWidth,
                          int height = kDefaultHeight);

  int image_width() const;
  int image_height() const;
  Color background_color() const;
  bool oversized() const;

private:
  Gtk::SpinButton width_;
  Gtk::SpinButton height_;
  Gtk::RadioButton white_;
  Gtk::RadioButton transparent_;
  Gtk::RadioButton custom_;
  Gtk::ColorButton color_;
};

}  // namespace lundukepaint

#endif
