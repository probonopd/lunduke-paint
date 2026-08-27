// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_UI_STATUS_BAR_HPP
#define BRUSHPAD_UI_STATUS_BAR_HPP

#include <glibmm/ustring.h>
#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/separator.h>

namespace brushpad {

class StatusBar : public Gtk::Box {
public:
  StatusBar();

  void set_hint(const Glib::ustring& hint);
  void show_coordinates(double x, double y);
  void clear_coordinates();
  void set_canvas_size(int width, int height);
  void set_zoom(double zoom);
  void set_modified(bool modified);
  void show_message(const Glib::ustring& message);

private:
  Gtk::Label hint_;
  Gtk::Label coords_;
  Gtk::Label size_;
  Gtk::Label zoom_;
  Gtk::Label modified_;
};

}  // namespace brushpad

#endif
