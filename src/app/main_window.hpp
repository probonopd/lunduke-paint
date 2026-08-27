// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_APP_MAIN_WINDOW_HPP
#define BRUSHPAD_APP_MAIN_WINDOW_HPP

#include "ui/canvas_view.hpp"
#include "ui/colors_panel.hpp"
#include "ui/history_panel.hpp"
#include "ui/layers_panel.hpp"
#include "ui/status_bar.hpp"
#include "ui/tool_options_bar.hpp"
#include "ui/toolbox.hpp"

#include <giomm/menumodel.h>
#include <glibmm/ustring.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/box.h>
#include <gtkmm/notebook.h>

namespace brushpad {

class MainWindow : public Gtk::ApplicationWindow {
public:
  MainWindow();

  void reset_canvas();
  void show_status(const Glib::ustring& message);

private:
  void build_ui();
  Glib::RefPtr<Gio::MenuModel> load_menubar_model();
  void on_toggle_right_dock();

  Gtk::Box root_{Gtk::ORIENTATION_VERTICAL};
  Gtk::Box toolbar_{Gtk::ORIENTATION_HORIZONTAL};
  ToolOptionsBar tool_options_bar_;
  Gtk::Box work_area_{Gtk::ORIENTATION_HORIZONTAL};
  Toolbox toolbox_;
  CanvasView canvas_;
  Gtk::Notebook right_dock_;
  ColorsPanel colors_panel_;
  LayersPanel layers_panel_;
  HistoryPanel history_panel_;
  StatusBar status_bar_;
};

}  // namespace brushpad

#endif
