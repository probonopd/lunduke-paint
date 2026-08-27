// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_UI_COLORS_PANEL_HPP
#define BRUSHPAD_UI_COLORS_PANEL_HPP

#include "raster/types.hpp"

#include <functional>
#include <gtkmm/box.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>

namespace brushpad {

class ColorsPanel : public Gtk::Box {
public:
  ColorsPanel();

  void set_colors(Color fg, Color bg);

  std::function<void(Color color, bool background)> on_swatch;

private:
  void add_swatch(Color color, const char* tip);
  bool on_swatch_draw(Gtk::DrawingArea* area, const Cairo::RefPtr<Cairo::Context>& cr, Color color);
  bool on_swatch_press(GdkEventButton* event, Color color);

  Gtk::Label heading_;
  Gtk::Grid grid_;
  Gtk::Label hint_;
  Color fg_ = Color::black();
  Color bg_ = Color::white();
  int col_ = 0;
  int row_ = 0;
};

}  // namespace brushpad

#endif
