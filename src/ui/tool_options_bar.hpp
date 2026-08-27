// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_UI_TOOL_OPTIONS_BAR_HPP
#define BRUSHPAD_UI_TOOL_OPTIONS_BAR_HPP

#include <gtkmm/box.h>
#include <gtkmm/label.h>

namespace brushpad {

class Tool;

class ToolOptionsBar : public Gtk::Box {
public:
  ToolOptionsBar();

  void show_tool(Tool* tool);

private:
  Gtk::Label placeholder_;
  Gtk::Widget* current_{nullptr};
};

}  // namespace brushpad

#endif
