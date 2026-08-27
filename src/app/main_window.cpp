// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/main_window.hpp"

#include "app/actions.hpp"
#include "tools/tools.hpp"

#include <glibmm/error.h>
#include <giomm/menu.h>
#include <gtkmm/builder.h>
#include <glibmm/miscutils.h>
#include <gtk/gtk.h>
#include <gtkmm/button.h>
#include <gtkmm/colorchooserdialog.h>
#include <gtkmm/menubar.h>
#include <gtkmm/separator.h>
#include <iostream>
#include <stdexcept>

namespace brushpad {
namespace {

Gtk::Button* toolbar_button(const char* icon, const char* tooltip, const char* action) {
  auto* button = Gtk::manage(new Gtk::Button());
  button->set_image_from_icon_name(icon, Gtk::ICON_SIZE_SMALL_TOOLBAR);
  button->set_tooltip_text(tooltip);
  gtk_actionable_set_action_name(GTK_ACTIONABLE(button->gobj()), action);
  button->set_can_focus(false);
  return button;
}

}  // namespace

MainWindow::MainWindow() {
  document_ = Document::create(kDefaultWidth, kDefaultHeight, Color::white());
  set_title("Untitled — Brushpad");
  set_default_size(1100, 720);
  // Traditional WM decorations: do not call set_titlebar() / GtkHeaderBar.

  add_action(actions::kToggleRightDock, sigc::mem_fun(*this, &MainWindow::on_toggle_right_dock));
  undo_action_ = add_action(actions::kUndo, sigc::mem_fun(*this, &MainWindow::on_undo));
  redo_action_ = add_action(actions::kRedo, sigc::mem_fun(*this, &MainWindow::on_redo));
  add_action(actions::kZoomIn, [this]() { canvas_.zoom_in_at(0, 0); });
  add_action(actions::kZoomOut, [this]() { canvas_.zoom_out_at(0, 0); });
  add_action(actions::kZoom100, [this]() { canvas_.set_zoom(1.0); });

  tools_.emplace_back(create_pencil_tool());
  tools_.emplace_back(create_brush_tool());
  tools_.emplace_back(create_eraser_tool());
  tools_.emplace_back(create_fill_tool());
  tools_.emplace_back(create_picker_tool());
  for (auto& tool : tools_) {
    tool->set_host(this);
  }
  active_tool_ = tools_.front().get();

  add_events(Gdk::KEY_PRESS_MASK | Gdk::KEY_RELEASE_MASK);
  signal_key_press_event().connect(sigc::mem_fun(*this, &MainWindow::on_key_press), false);
  signal_key_release_event().connect(sigc::mem_fun(*this, &MainWindow::on_key_release), false);

  build_ui();
  bind_document();
  set_active_tool("pencil");
  show_all();
  update_chrome();
}

MainWindow::~MainWindow() = default;

void MainWindow::build_ui() {
  auto model = load_menubar_model();
  auto* menubar = Gtk::make_managed<Gtk::MenuBar>(model);

  build_toolbar();

  canvas_.set_hexpand(true);
  canvas_.set_vexpand(true);

  toolbox_.add_tool_button("pencil", "Pencil (P)", "document-edit-symbolic");
  toolbox_.add_tool_button("brush", "Brush (B)", "edit-select-symbolic");
  toolbox_.add_tool_button("eraser", "Eraser (A)", "edit-clear-symbolic");
  toolbox_.add_tool_button("fill", "Flood fill (F)", "color-fill-symbolic");
  toolbox_.add_tool_button("picker", "Color picker (C)", "color-select-symbolic");
  toolbox_.on_tool_chosen = [this](const std::string& id) { set_active_tool(id); };
  toolbox_.on_well_clicked = [this](bool background) { choose_color(background); };
  colors_panel_.on_swatch = [this](Color color, bool background) {
    if (background) {
      document_->set_background(color);
    } else {
      document_->set_foreground(color);
    }
  };

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
  canvas_.signal_view_changed().connect([this]() { update_chrome(); });

  add(root_);
}

void MainWindow::build_toolbar() {
  toolbar_.set_spacing(4);
  toolbar_.set_border_width(4);
  toolbar_.get_style_context()->add_class("toolbar");
  toolbar_.set_size_request(-1, 36);

  toolbar_.pack_start(*toolbar_button("document-new", "New", "app.new"), Gtk::PACK_SHRINK);
  toolbar_.pack_start(*toolbar_button("document-open", "Open", "app.open"), Gtk::PACK_SHRINK);
  toolbar_.pack_start(*toolbar_button("document-save", "Save", "app.save"), Gtk::PACK_SHRINK);
  toolbar_.pack_start(*Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_VERTICAL)), Gtk::PACK_SHRINK);
  toolbar_.pack_start(*toolbar_button("edit-undo", "Undo", "win.undo"), Gtk::PACK_SHRINK);
  toolbar_.pack_start(*toolbar_button("edit-redo", "Redo", "win.redo"), Gtk::PACK_SHRINK);
  toolbar_.pack_start(*Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_VERTICAL)), Gtk::PACK_SHRINK);
  toolbar_.pack_start(*toolbar_button("zoom-out", "Zoom out", "win.zoom-out"), Gtk::PACK_SHRINK);
  toolbar_.pack_start(*toolbar_button("zoom-original", "100%", "win.zoom-100"), Gtk::PACK_SHRINK);
  toolbar_.pack_start(*toolbar_button("zoom-in", "Zoom in", "win.zoom-in"), Gtk::PACK_SHRINK);
  toolbar_.pack_start(*Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_VERTICAL)), Gtk::PACK_SHRINK);
  toolbar_.pack_start(*toolbar_button("view-sidebar-end-symbolic", "Right dock (F12)",
                                     "win.toggle-right-dock"),
                      Gtk::PACK_SHRINK);
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

void MainWindow::bind_document() {
  canvas_.set_document(document_.get());
  canvas_.set_tool(active_tool_);
  document_->set_on_changed([this]() { update_chrome(); });
  document_->set_on_invalidated([this](Rect rect) { canvas_.invalidate_rect(rect); });
  toolbox_.set_colors(document_->foreground(), document_->background());
}

void MainWindow::reset_canvas() {
  new_document(kDefaultWidth, kDefaultHeight, Color::white());
}

void MainWindow::new_document(int width, int height, Color background) {
  if (active_tool_ != nullptr) {
    active_tool_->on_cancel();
  }
  document_ = Document::create(width, height, background);
  bind_document();
  update_chrome();
}

void MainWindow::show_status(const Glib::ustring& message) {
  status_bar_.show_message(message);
}

void MainWindow::set_active_tool(const std::string& id) {
  Tool* found = nullptr;
  for (auto& tool : tools_) {
    if (id == tool->id()) {
      found = tool.get();
      break;
    }
  }
  if (found == nullptr || found == active_tool_) {
    if (found != nullptr) {
      toolbox_.set_active_tool(id);
      tool_options_bar_.show_tool(found);
    }
    return;
  }
  if (active_tool_ != nullptr) {
    active_tool_->on_cancel();
    previous_tool_ = active_tool_;
  }
  active_tool_ = found;
  canvas_.set_tool(active_tool_);
  toolbox_.set_active_tool(id);
  tool_options_bar_.show_tool(active_tool_);
  status_bar_.set_hint(active_tool_->hint());
}

void MainWindow::set_stroke_size(int size) {
  if (size < 1) {
    size = 1;
  }
  stroke_size_ = size;
}

void MainWindow::set_brush_antialias(bool enabled) {
  brush_aa_ = enabled;
}

void MainWindow::set_fill_tolerance(int tolerance) {
  if (tolerance < 0) {
    tolerance = 0;
  }
  fill_tolerance_ = tolerance;
}

void MainWindow::invalidate_canvas(Rect rect) {
  canvas_.invalidate_rect(rect);
}

void MainWindow::return_to_previous_tool() {
  if (previous_tool_ != nullptr) {
    set_active_tool(previous_tool_->id());
  }
}

Color MainWindow::sample_canvas(int x, int y) const {
  return canvas_.sample_pixel(x, y);
}

void MainWindow::on_undo() {
  if (active_tool_ != nullptr) {
    active_tool_->on_cancel();
  }
  document_->undo();
}

void MainWindow::on_redo() {
  if (active_tool_ != nullptr) {
    active_tool_->on_cancel();
  }
  document_->redo();
}

void MainWindow::update_chrome() {
  update_title();
  undo_action_->set_enabled(document_->history().can_undo());
  redo_action_->set_enabled(document_->history().can_redo());
  status_bar_.set_canvas_size(document_->width(), document_->height());
  status_bar_.set_zoom(canvas_.zoom());
  status_bar_.set_modified(document_->dirty());
  if (active_tool_ != nullptr) {
    status_bar_.set_hint(active_tool_->hint());
  }
  toolbox_.set_colors(document_->foreground(), document_->background());
}

void MainWindow::update_title() {
  Glib::ustring name = "Untitled";
  if (!document_->path().empty()) {
    name = Glib::path_get_basename(document_->path());
  }
  if (document_->dirty()) {
    name += "*";
  }
  set_title(name + " — Brushpad");
}

void MainWindow::choose_color(bool background) {
  Gtk::ColorChooserDialog dialog(background ? "Background color" : "Foreground color");
  dialog.set_transient_for(*this);
  dialog.set_use_alpha(true);
  const Color current = background ? document_->background() : document_->foreground();
  Gdk::RGBA rgba;
  rgba.set_rgba(current.r / 255.0, current.g / 255.0, current.b / 255.0, current.a / 255.0);
  dialog.set_rgba(rgba);
  if (dialog.run() != Gtk::RESPONSE_OK) {
    return;
  }
  const Gdk::RGBA chosen = dialog.get_rgba();
  Color color;
  color.r = static_cast<std::uint8_t>(chosen.get_red() * 255.0 + 0.5);
  color.g = static_cast<std::uint8_t>(chosen.get_green() * 255.0 + 0.5);
  color.b = static_cast<std::uint8_t>(chosen.get_blue() * 255.0 + 0.5);
  color.a = static_cast<std::uint8_t>(chosen.get_alpha() * 255.0 + 0.5);
  if (background) {
    document_->set_background(color);
  } else {
    document_->set_foreground(color);
  }
}

bool MainWindow::focus_is_editable() const {
  auto* focus = get_focus();
  if (focus == nullptr) {
    return false;
  }
  return GTK_IS_EDITABLE(focus->gobj());
}

bool MainWindow::on_key_press(GdkEventKey* event) {
  if (event == nullptr) {
    return false;
  }
  if (event->keyval == GDK_KEY_space) {
    canvas_.set_space_down(true);
    return false;
  }
  if (focus_is_editable()) {
    return false;
  }
  if (event->keyval == GDK_KEY_Escape) {
    if (active_tool_ != nullptr) {
      active_tool_->on_cancel();
    }
    return true;
  }
  if ((event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)) != 0) {
    return false;
  }
  const guint32 ch = gdk_keyval_to_unicode(gdk_keyval_to_upper(event->keyval));
  if (ch == 'X') {
    document_->swap_colors();
    return true;
  }
  if (ch == 'D') {
    document_->reset_colors();
    return true;
  }
  for (auto& tool : tools_) {
    if (tool->shortcut() == static_cast<char>(ch)) {
      set_active_tool(tool->id());
      return true;
    }
  }
  return false;
}

bool MainWindow::on_key_release(GdkEventKey* event) {
  if (event != nullptr && event->keyval == GDK_KEY_space) {
    canvas_.set_space_down(false);
  }
  return false;
}

void MainWindow::on_toggle_right_dock() {
  right_dock_.set_visible(!right_dock_.get_visible());
}

}  // namespace brushpad
