// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_UI_TOOLBOX_HPP
#define BRUSHPAD_UI_TOOLBOX_HPP

#include <functional>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/grid.h>
#include <string>
#include <vector>

#include "raster/types.hpp"

namespace brushpad {

class Toolbox : public Gtk::Box {
public:
  Toolbox();

  void add_tool_button(const std::string& id, const std::string& tooltip, const std::string& icon_name);
  void set_active_tool(const std::string& id);
  void set_colors(Color fg, Color bg);

  std::function<void(const std::string& id)> on_tool_chosen;
  std::function<void(bool background)> on_well_clicked;
  std::function<void(bool background)> on_transparent;

private:
  bool on_wells_draw(const Cairo::RefPtr<Cairo::Context>& cr);
  bool on_wells_press(GdkEventButton* event);
  bool on_trans_draw(const Cairo::RefPtr<Cairo::Context>& cr);
  bool on_trans_press(GdkEventButton* event);

  Gtk::Grid grid_;
  Gtk::DrawingArea wells_;
  Gtk::DrawingArea trans_;
  std::vector<Gtk::Button*> buttons_;
  std::vector<std::string> ids_;
  Color fg_ = Color::black();
  Color bg_ = Color::white();
  int next_col_ = 0;
  int next_row_ = 0;
};

}  // namespace brushpad

#endif
