// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/main_window.hpp"

#include "app/actions.hpp"
#include "io/image_io.hpp"
#include "ui/dialogs_new.hpp"
#include "tools/tools.hpp"

#include <glibmm/error.h>
#include <giomm/menu.h>
#include <gtkmm/builder.h>
#include <glibmm/miscutils.h>
#include <gtk/gtk.h>
#include <gtkmm/button.h>
#include <gtkmm/colorchooserdialog.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/filefilter.h>
#include <gtkmm/messagedialog.h>
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
  add_action(actions::kZoomIn, [this]() { canvas_.zoom_in(); });
  add_action(actions::kZoomOut, [this]() { canvas_.zoom_out(); });
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

void MainWindow::action_new() {
  if (!confirm_lose_changes()) {
    return;
  }
  NewImageDialog dialog(*this);
  if (dialog.run() != Gtk::RESPONSE_OK) {
    return;
  }
  const int width = dialog.image_width();
  const int height = dialog.image_height();
  if (width < 1 || height < 1) {
    return;
  }
  if (width > kHardMaxSide || height > kHardMaxSide) {
    Gtk::MessageDialog refuse(*this, "Images cannot be larger than 16384 pixels on a side.", false,
                              Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
    refuse.run();
    return;
  }
  if (dialog.oversized()) {
    Gtk::MessageDialog warn(*this,
                            "This canvas is larger than 8192 pixels on a side and may use a lot of memory.",
                            false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL, true);
    if (warn.run() != Gtk::RESPONSE_OK) {
      return;
    }
  }
  new_document(width, height, dialog.background_color());
  show_status("New canvas");
}

void MainWindow::action_open() {
  if (!confirm_lose_changes()) {
    return;
  }
  const std::string path = choose_open_path();
  if (path.empty()) {
    return;
  }
  LoadedImage loaded = load_flat_image(path);
  if (!loaded.ok()) {
    Gtk::MessageDialog err(*this, "Could not open image.", false, Gtk::MESSAGE_ERROR,
                           Gtk::BUTTONS_OK, true);
    err.set_secondary_text(loaded.error);
    err.run();
    return;
  }
  if (loaded.width > kSoftMaxSide || loaded.height > kSoftMaxSide) {
    Gtk::MessageDialog warn(*this,
                            "This image is larger than 8192 pixels on a side and may use a lot of memory.",
                            false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL, true);
    if (warn.run() != Gtk::RESPONSE_OK) {
      return;
    }
  }
  if (active_tool_ != nullptr) {
    active_tool_->on_cancel();
  }
  document_ = Document::create(loaded.width, loaded.height, Color::transparent(), loaded.layer_name);
  document_->layers().active_layer().write_rect(Rect{0, 0, loaded.width, loaded.height},
                                                loaded.rgba.data());
  document_->set_path(path);
  document_->mark_clean();
  bind_document();
  update_chrome();
  show_status("Opened " + Glib::path_get_basename(path));
}

void MainWindow::action_save() {
  if (document_->path().empty() || format_from_path(document_->path()) == ImageFormat::Unknown) {
    action_save_as();
    return;
  }
  if (save_to_path(document_->path(), format_from_path(document_->path()))) {
    document_->mark_clean();
    update_chrome();
    show_status("Saved");
  }
}

void MainWindow::action_save_as() {
  std::string path;
  ImageFormat format = ImageFormat::Png;
  if (!choose_save_path(path, format)) {
    return;
  }
  if (save_to_path(path, format)) {
    document_->set_path(path);
    document_->mark_clean();
    update_chrome();
    show_status("Saved");
  }
}

bool MainWindow::confirm_close() {
  return confirm_lose_changes();
}

bool MainWindow::on_delete_event(GdkEventAny* event) {
  if (!confirm_close()) {
    return true;
  }
  return Gtk::ApplicationWindow::on_delete_event(event);
}

bool MainWindow::confirm_lose_changes() {
  if (!document_->dirty()) {
    return true;
  }
  Gtk::MessageDialog dialog(*this, "Save changes to the current image?", false,
                            Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE, true);
  dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dialog.add_button("_Discard", Gtk::RESPONSE_NO);
  dialog.add_button("_Save", Gtk::RESPONSE_YES);
  const int response = dialog.run();
  if (response == Gtk::RESPONSE_CANCEL) {
    return false;
  }
  if (response == Gtk::RESPONSE_YES) {
    action_save();
    return !document_->dirty();
  }
  return true;
}

bool MainWindow::layer_has_transparency() const {
  const Layer& layer = document_->layers().active_layer();
  const std::uint8_t* p = layer.pixels();
  const int n = layer.width() * layer.height();
  for (int i = 0; i < n; ++i) {
    if (p[static_cast<std::size_t>(i) * 4 + 3] != 255) {
      return true;
    }
  }
  return false;
}

bool MainWindow::save_to_path(const std::string& path, ImageFormat format) {
  if (format == ImageFormat::Jpeg && layer_has_transparency()) {
    Gtk::MessageDialog warn(*this,
                            "JPEG cannot store transparency. The image will be flattened onto white.",
                            false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL, true);
    if (warn.run() != Gtk::RESPONSE_OK) {
      return false;
    }
  }
  if (format == ImageFormat::Bmp && layer_has_transparency()) {
    Gtk::MessageDialog warn(*this,
                            "BMP cannot store transparency. The image will be flattened onto white.",
                            false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL, true);
    if (warn.run() != Gtk::RESPONSE_OK) {
      return false;
    }
  }
  const Layer& layer = document_->layers().active_layer();
  std::string error;
  if (!save_flat_image(path, format, layer.pixels(), layer.width(), layer.height(), layer.stride(),
                       90, error)) {
    Gtk::MessageDialog err(*this, "Could not save image.", false, Gtk::MESSAGE_ERROR,
                           Gtk::BUTTONS_OK, true);
    err.set_secondary_text(error);
    err.run();
    return false;
  }
  return true;
}

std::string MainWindow::choose_open_path() {
  Gtk::FileChooserDialog dialog(*this, "Open Image", Gtk::FILE_CHOOSER_ACTION_OPEN);
  dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dialog.add_button("_Open", Gtk::RESPONSE_ACCEPT);
  auto all = Gtk::FileFilter::create();
  all->set_name("Images");
  all->add_pattern("*.png");
  all->add_pattern("*.jpg");
  all->add_pattern("*.jpeg");
  all->add_pattern("*.bmp");
  dialog.add_filter(all);
  auto png = Gtk::FileFilter::create();
  png->set_name("PNG");
  png->add_pattern("*.png");
  dialog.add_filter(png);
  auto jpeg = Gtk::FileFilter::create();
  jpeg->set_name("JPEG");
  jpeg->add_pattern("*.jpg");
  jpeg->add_pattern("*.jpeg");
  dialog.add_filter(jpeg);
  auto bmp = Gtk::FileFilter::create();
  bmp->set_name("BMP");
  bmp->add_pattern("*.bmp");
  dialog.add_filter(bmp);
  if (dialog.run() != Gtk::RESPONSE_ACCEPT) {
    return {};
  }
  return dialog.get_filename();
}

bool MainWindow::choose_save_path(std::string& path, ImageFormat& format) {
  Gtk::FileChooserDialog dialog(*this, "Save Image", Gtk::FILE_CHOOSER_ACTION_SAVE);
  dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dialog.add_button("_Save", Gtk::RESPONSE_ACCEPT);
  dialog.set_do_overwrite_confirmation(true);
  auto png = Gtk::FileFilter::create();
  png->set_name("PNG image (*.png)");
  png->add_pattern("*.png");
  dialog.add_filter(png);
  auto jpeg = Gtk::FileFilter::create();
  jpeg->set_name("JPEG image (*.jpg)");
  jpeg->add_pattern("*.jpg");
  jpeg->add_pattern("*.jpeg");
  dialog.add_filter(jpeg);
  auto bmp = Gtk::FileFilter::create();
  bmp->set_name("BMP image (*.bmp)");
  bmp->add_pattern("*.bmp");
  dialog.add_filter(bmp);
  if (!document_->path().empty()) {
    dialog.set_filename(document_->path());
  } else {
    dialog.set_current_name("untitled.png");
  }
  if (dialog.run() != Gtk::RESPONSE_ACCEPT) {
    return false;
  }
  path = dialog.get_filename();
  format = format_from_path(path);
  if (format == ImageFormat::Unknown) {
    auto filter = dialog.get_filter();
    if (filter && filter->get_name().find("JPEG") != Glib::ustring::npos) {
      format = ImageFormat::Jpeg;
    } else if (filter && filter->get_name().find("BMP") != Glib::ustring::npos) {
      format = ImageFormat::Bmp;
    } else {
      format = ImageFormat::Png;
    }
    path += format_extension(format);
  }
  return true;
}

}  // namespace brushpad
