// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_UI_STATUS_BAR_HPP
#define BRUSHPAD_UI_STATUS_BAR_HPP

#include <glibmm/ustring.h>
#include <gtkmm/statusbar.h>

namespace brushpad {

class StatusBar : public Gtk::Statusbar {
public:
  StatusBar();

  void show_coordinates(double x, double y);
  void clear_coordinates();
  void show_message(const Glib::ustring& message);

private:
  guint context_id_{0};
};

}  // namespace brushpad

#endif
