// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_UI_TOOLBOX_HPP
#define BRUSHPAD_UI_TOOLBOX_HPP

#include <functional>
#include <glibmm/refptr.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <string>
#include <vector>

#include "raster/types.hpp"
#include "ui/tool_selection.hpp"

namespace brushpad {

class Toolbox : public Gtk::Box {
public:
  Toolbox();

  void add_tool_button(const std::string& id, const std::string& tooltip, const std::string& icon_name);
  void set_active_tool(const std::string& id);
  void set_colors(Color fg, Color bg);

  const std::string& active_tool_id() const { return selection_.active_id(); }
  // True when the button for id currently carries the selected-tool highlight.
  bool tool_button_selected(const std::string& id) const;

  std::function<void(const std::string& id)> on_tool_chosen;
  std::function<void(bool background)> on_well_clicked;
  std::function<void(bool background)> on_transparent;

private:
  static void ensure_css();
  void apply_selection_style();
  bool on_wells_draw(const Cairo::RefPtr<Cairo::Context>& cr);
  bool on_wells_press(GdkEventButton* event);
  bool on_trans_draw(const Cairo::RefPtr<Cairo::Context>& cr);
  bool on_trans_press(GdkEventButton* event);

  Gtk::Grid grid_;
  Gtk::DrawingArea wells_;
  Gtk::Box well_labels_{Gtk::ORIENTATION_VERTICAL, 0};
  Gtk::Label fg_label_{"Foreground"};
  Gtk::Label bg_label_{"Background"};
  Gtk::DrawingArea trans_;
  std::vector<Gtk::Button*> buttons_;
  ToolSelection selection_;
  Color fg_ = Color::black();
  Color bg_ = Color::white();
  int next_col_ = 0;
  int next_row_ = 0;
};

}  // namespace brushpad

#endif
