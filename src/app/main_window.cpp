// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/main_window.hpp"

#include "app/actions.hpp"

#include <glibmm/error.h>
#include <giomm/menu.h>
#include <gtkmm/builder.h>
#include <gtkmm/menubar.h>
#include <gtkmm/separator.h>
#include <iostream>
#include <stdexcept>

namespace brushpad {

MainWindow::MainWindow() {
  set_title("Brushpad");
  set_default_size(1100, 720);
  // Traditional WM decorations: do not call set_titlebar() / GtkHeaderBar.

  add_action(actions::kToggleRightDock,
             sigc::mem_fun(*this, &MainWindow::on_toggle_right_dock));

  build_ui();
  show_all();
}

void MainWindow::build_ui() {
  auto model = load_menubar_model();
  auto* menubar = Gtk::make_managed<Gtk::MenuBar>(model);

  toolbar_.set_spacing(4);
  toolbar_.set_border_width(4);
  toolbar_.get_style_context()->add_class("toolbar");
  toolbar_.set_size_request(-1, 36);

  canvas_.set_hexpand(true);
  canvas_.set_vexpand(true);

  right_dock_.append_page(colors_panel_, "Colors");
  right_dock_.append_page(layers_panel_, "Layers");
  right_dock_.append_page(history_panel_, "History");
  right_dock_.set_size_request(220, -1);

  auto* left_sep = Gtk::make_managed<Gtk::Separator>(Gtk::ORIENTATION_VERTICAL);
  auto* right_sep = Gtk::make_managed<Gtk::Separator>(Gtk::ORIENTATION_VERTICAL);

  work_area_.pack_start(toolbox_, Gtk::PACK_SHRINK);
  work_area_.pack_start(*left_sep, Gtk::PACK_SHRINK);
  work_area_.pack_start(canvas_, Gtk::PACK_EXPAND_WIDGET);
  work_area_.pack_start(*right_sep, Gtk::PACK_SHRINK);
  work_area_.pack_start(right_dock_, Gtk::PACK_SHRINK);
  work_area_.set_hexpand(true);
  work_area_.set_vexpand(true);

  root_.pack_start(*menubar, Gtk::PACK_SHRINK);
  root_.pack_start(toolbar_, Gtk::PACK_SHRINK);
  root_.pack_start(tool_options_bar_, Gtk::PACK_SHRINK);
  root_.pack_start(work_area_, Gtk::PACK_EXPAND_WIDGET);
  root_.pack_start(status_bar_, Gtk::PACK_SHRINK);

  canvas_.signal_pointer_moved().connect(
      sigc::mem_fun(status_bar_, &StatusBar::show_coordinates));
  canvas_.signal_pointer_left().connect(
      sigc::mem_fun(status_bar_, &StatusBar::clear_coordinates));

  add(root_);
}

Glib::RefPtr<Gio::MenuModel> MainWindow::load_menubar_model() {
  try {
    auto builder = Gtk::Builder::create_from_resource(
        "/org/brushpad/Brushpad/ui/menus.xml");
    auto object = builder->get_object("menubar");
    auto menu = Glib::RefPtr<Gio::Menu>::cast_dynamic(object);
    if (!menu) {
      throw std::runtime_error("menus.xml is missing the 'menubar' object");
    }
    return menu;
  } catch (const Glib::Error& error) {
    std::cerr << "Failed to load menus.xml: " << error.what() << '\n';
    throw;
  }
}

void MainWindow::reset_canvas() {
  canvas_.reset_blank();
}

void MainWindow::show_status(const Glib::ustring& message) {
  status_bar_.show_message(message);
}

void MainWindow::on_toggle_right_dock() {
  right_dock_.set_visible(!right_dock_.get_visible());
}

}  // namespace brushpad
