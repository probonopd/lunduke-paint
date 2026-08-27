// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/main_window.hpp"

#include "app/actions.hpp"
#include "doc/commands_image.hpp"
#include "doc/commands_layers.hpp"
#include "doc/commands_pixels.hpp"
#include "doc/selection.hpp"
#include "io/image_io.hpp"
#include "io/ora.hpp"
#include "raster/transform.hpp"
#include "ui/dialogs_image.hpp"
#include "ui/dialogs_new.hpp"
#include "tools/tools.hpp"

#include <glibmm/error.h>
#include <giomm/menu.h>
#include <gtkmm/builder.h>
#include <glibmm/miscutils.h>
#include <gtk/gtk.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm/button.h>
#include <gtkmm/clipboard.h>
#include <gtkmm/colorchooserdialog.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/filefilter.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/menubar.h>
#include <gtkmm/separator.h>
#include <cstring>
#include <functional>
#include <iostream>
#include <vector>
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
  add_action(actions::kZoomFit, sigc::mem_fun(*this, &MainWindow::action_zoom_fit));
  add_action(actions::kToggleGrid, sigc::mem_fun(*this, &MainWindow::action_toggle_grid));
  cut_action_ = add_action(actions::kCut, sigc::mem_fun(*this, &MainWindow::action_cut));
  copy_action_ = add_action(actions::kCopy, sigc::mem_fun(*this, &MainWindow::action_copy));
  add_action(actions::kPaste, sigc::mem_fun(*this, &MainWindow::action_paste));
  delete_action_ = add_action(actions::kDelete, sigc::mem_fun(*this, &MainWindow::action_delete));
  duplicate_action_ = add_action(actions::kDuplicate, sigc::mem_fun(*this, &MainWindow::action_duplicate));
  add_action(actions::kSelectAll, sigc::mem_fun(*this, &MainWindow::action_select_all));
  deselect_action_ = add_action(actions::kDeselect, sigc::mem_fun(*this, &MainWindow::action_deselect));
  invert_action_ = add_action(actions::kInvertSelection,
                             sigc::mem_fun(*this, &MainWindow::action_invert_selection));
  add_action(actions::kCanvasSize, sigc::mem_fun(*this, &MainWindow::action_canvas_size));
  add_action(actions::kScale, sigc::mem_fun(*this, &MainWindow::action_scale));
  crop_action_ = add_action(actions::kCrop, sigc::mem_fun(*this, &MainWindow::action_crop));
  add_action(actions::kAutocrop, sigc::mem_fun(*this, &MainWindow::action_autocrop));
  add_action(actions::kRotate90, sigc::mem_fun(*this, &MainWindow::action_rotate_90));
  add_action(actions::kRotate180, sigc::mem_fun(*this, &MainWindow::action_rotate_180));
  add_action(actions::kFlipH, sigc::mem_fun(*this, &MainWindow::action_flip_h));
  add_action(actions::kFlipV, sigc::mem_fun(*this, &MainWindow::action_flip_v));
  add_action(actions::kClear, sigc::mem_fun(*this, &MainWindow::action_clear));
  add_action("print")->set_enabled(false);
  add_action(actions::kLayerNew, sigc::mem_fun(*this, &MainWindow::action_layer_new));
  add_action(actions::kLayerDuplicate, sigc::mem_fun(*this, &MainWindow::action_layer_duplicate));
  layer_delete_action_ = add_action(actions::kLayerDelete,
                                    sigc::mem_fun(*this, &MainWindow::action_layer_delete));
  layer_raise_action_ = add_action(actions::kLayerRaise,
                                   sigc::mem_fun(*this, &MainWindow::action_layer_raise));
  layer_lower_action_ = add_action(actions::kLayerLower,
                                   sigc::mem_fun(*this, &MainWindow::action_layer_lower));
  layer_merge_action_ = add_action(actions::kLayerMergeDown,
                                   sigc::mem_fun(*this, &MainWindow::action_layer_merge_down));
  layer_flatten_action_ = add_action(actions::kLayerFlatten,
                                     sigc::mem_fun(*this, &MainWindow::action_layer_flatten));
  add_action(actions::kLayerProperties, sigc::mem_fun(*this, &MainWindow::action_layer_properties));
  add_action("adjust-brightness")->set_enabled(false);
  add_action("effect-blur")->set_enabled(false);
  add_action("about")->set_enabled(false);

  tools_.emplace_back(create_rect_select_tool());
  tools_.emplace_back(create_pencil_tool());
  tools_.emplace_back(create_brush_tool());
  tools_.emplace_back(create_eraser_tool());
  tools_.emplace_back(create_fill_tool());
  tools_.emplace_back(create_picker_tool());
  tools_.emplace_back(create_line_tool());
  tools_.emplace_back(create_rectangle_tool());
  tools_.emplace_back(create_ellipse_tool());
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

  toolbox_.add_tool_button("rect-select", "Rectangle select (S)", "edit-select-all-symbolic");
  toolbox_.add_tool_button("pencil", "Pencil (P)", "document-edit-symbolic");
  toolbox_.add_tool_button("brush", "Brush (B)", "edit-select-symbolic");
  toolbox_.add_tool_button("eraser", "Eraser (A)", "edit-clear-symbolic");
  toolbox_.add_tool_button("fill", "Flood fill (F)", "color-fill-symbolic");
  toolbox_.add_tool_button("picker", "Color picker (C)", "color-select-symbolic");
  toolbox_.add_tool_button("line", "Line (L)", "insert-link-symbolic");
  toolbox_.add_tool_button("rectangle", "Rectangle (R)", "view-restore-symbolic");
  toolbox_.add_tool_button("ellipse", "Ellipse (E)", "media-record-symbolic");
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
  toolbar_.pack_start(*toolbar_button("edit-cut", "Cut", "win.cut"), Gtk::PACK_SHRINK);
  toolbar_.pack_start(*toolbar_button("edit-copy", "Copy", "win.copy"), Gtk::PACK_SHRINK);
  toolbar_.pack_start(*toolbar_button("edit-paste", "Paste", "win.paste"), Gtk::PACK_SHRINK);
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
  layers_panel_.set_document(document_.get());
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

void MainWindow::show_status_hint(const char* message) {
  if (message != nullptr) {
    show_status(message);
  }
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
  const Selection& sel = document_->selection();
  const bool has_sel = !sel.empty();
  cut_action_->set_enabled(has_sel);
  copy_action_->set_enabled(has_sel);
  delete_action_->set_enabled(has_sel);
  duplicate_action_->set_enabled(has_sel && !sel.inverted());
  deselect_action_->set_enabled(has_sel);
  invert_action_->set_enabled(true);
  crop_action_->set_enabled(has_sel && !sel.inverted());
  if (has_sel) {
    const Rect b = sel.bounds();
    status_bar_.set_selection_size(b.w, b.h, true);
  } else {
    status_bar_.set_selection_size(0, 0, false);
  }
  status_bar_.set_canvas_size(document_->width(), document_->height());
  status_bar_.set_zoom(canvas_.zoom());
  status_bar_.set_modified(document_->dirty());
  if (active_tool_ != nullptr) {
    status_bar_.set_hint(active_tool_->hint());
  }
  toolbox_.set_colors(document_->foreground(), document_->background());
  canvas_.refresh_size();
  layers_panel_.refresh();
  const int nlayers = document_->layers().count();
  const int active = document_->layers().active_index();
  layer_delete_action_->set_enabled(nlayers > 1);
  layer_raise_action_->set_enabled(active + 1 < nlayers);
  layer_lower_action_->set_enabled(active > 0);
  layer_merge_action_->set_enabled(active > 0);
  layer_flatten_action_->set_enabled(nlayers > 1);
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
    if (active_tool_ != nullptr && active_tool_->is_stroking()) {
      active_tool_->on_cancel();
      return true;
    }
    action_deselect();
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
  if (active_tool_ != nullptr) {
    active_tool_->on_cancel();
  }
  if (format_from_path(path) == ImageFormat::Ora) {
    LoadedOra loaded = load_ora(path);
    if (!loaded.ok()) {
      Gtk::MessageDialog err(*this, "Could not open OpenRaster file.", false, Gtk::MESSAGE_ERROR,
                             Gtk::BUTTONS_OK, true);
      err.set_secondary_text(loaded.error);
      err.run();
      return;
    }
    if (loaded.warn_size) {
      Gtk::MessageDialog warn(*this,
                              "This image is larger than 8192 pixels on a side and may use a lot of memory.",
                              false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL, true);
      if (warn.run() != Gtk::RESPONSE_OK) {
        return;
      }
    }
    if (loaded.warn_layers) {
      Gtk::MessageDialog warn(*this, "This file has more than 64 layers and may use a lot of memory.",
                              false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL, true);
      if (warn.run() != Gtk::RESPONSE_OK) {
        return;
      }
    }
    std::vector<std::unique_ptr<Layer>> layers;
    layers.reserve(loaded.layers.size());
    for (const auto& snap : loaded.layers) {
      layers.push_back(layer_from_snapshot(snap));
    }
    document_ = Document::create(loaded.width, loaded.height, Color::transparent(),
                                 loaded.layers.front().name);
    document_->replace_stack(loaded.width, loaded.height, std::move(layers),
                             static_cast<int>(loaded.layers.size()) - 1);
    document_->set_path(path);
    document_->mark_clean();
    bind_document();
    update_chrome();
    show_status("Opened " + Glib::path_get_basename(path));
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
  ImageFormat format = ImageFormat::Ora;
  if (!choose_save_path(path, format)) {
    return;
  }
  bool also_ora = false;
  if (format != ImageFormat::Ora && document_->layers().count() > 1 && !flatten_ora_offered_) {
    Gtk::MessageDialog warn(*this,
                            "This document has multiple layers. Saving a flat file will flatten visible layers.",
                            false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_NONE, true);
    warn.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    warn.add_button("Flatten only", Gtk::RESPONSE_NO);
    warn.add_button("Flatten and keep .ora", Gtk::RESPONSE_YES);
    const int response = warn.run();
    if (response == Gtk::RESPONSE_CANCEL) {
      return;
    }
    also_ora = response == Gtk::RESPONSE_YES;
    flatten_ora_offered_ = true;
  }
  if (save_to_path(path, format)) {
    if (also_ora) {
      std::string ora_path = path;
      auto dot = ora_path.find_last_of('.');
      if (dot != std::string::npos) {
        ora_path = ora_path.substr(0, dot);
      }
      ora_path += ".ora";
      std::string error;
      if (!save_ora(ora_path, *document_, error)) {
        Gtk::MessageDialog err(*this, "Saved the flat file, but could not write the .ora copy.",
                               false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
        err.set_secondary_text(error);
        err.run();
      }
    }
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
  std::vector<std::uint8_t> flat;
  composite_visible(flat);
  const int n = document_->width() * document_->height();
  for (int i = 0; i < n; ++i) {
    if (flat[static_cast<std::size_t>(i) * 4 + 3] != 255) {
      return true;
    }
  }
  return false;
}

void MainWindow::composite_visible(std::vector<std::uint8_t>& dest) const {
  const int w = document_->width();
  const int h = document_->height();
  dest.assign(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4, 0);
  document_->layers().composite_rect(dest.data(), w * 4, Rect{0, 0, w, h});
}

bool MainWindow::save_to_path(const std::string& path, ImageFormat format) {
  if (format == ImageFormat::Ora) {
    std::string error;
    if (!save_ora(path, *document_, error)) {
      Gtk::MessageDialog err(*this, "Could not save OpenRaster file.", false, Gtk::MESSAGE_ERROR,
                             Gtk::BUTTONS_OK, true);
      err.set_secondary_text(error);
      err.run();
      return false;
    }
    return true;
  }

  const bool multi = document_->layers().count() > 1;
  if (multi && format == ImageFormat::Png) {
    Gtk::MessageDialog warn(*this,
                            "PNG will flatten visible layers (alpha is kept).",
                            false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL, true);
    if (warn.run() != Gtk::RESPONSE_OK) {
      return false;
    }
  }
  if (format == ImageFormat::Jpeg && (layer_has_transparency() || multi)) {
    Gtk::MessageDialog warn(*this,
                            "JPEG cannot store transparency or layers. The image will be flattened onto white.",
                            false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL, true);
    if (warn.run() != Gtk::RESPONSE_OK) {
      return false;
    }
  }
  if (format == ImageFormat::Bmp && (layer_has_transparency() || multi)) {
    Gtk::MessageDialog warn(*this,
                            "BMP cannot store transparency or layers. The image will be flattened onto white.",
                            false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL, true);
    if (warn.run() != Gtk::RESPONSE_OK) {
      return false;
    }
  }
  std::vector<std::uint8_t> flat;
  composite_visible(flat);
  std::string error;
  if (!save_flat_image(path, format, flat.data(), document_->width(), document_->height(),
                       document_->width() * 4, 90, error)) {
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
  all->add_pattern("*.ora");
  all->add_pattern("*.png");
  all->add_pattern("*.jpg");
  all->add_pattern("*.jpeg");
  all->add_pattern("*.bmp");
  dialog.add_filter(all);
  auto ora = Gtk::FileFilter::create();
  ora->set_name("OpenRaster (*.ora)");
  ora->add_pattern("*.ora");
  dialog.add_filter(ora);
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
  auto ora = Gtk::FileFilter::create();
  ora->set_name("OpenRaster project (*.ora)");
  ora->add_pattern("*.ora");
  dialog.add_filter(ora);
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
    dialog.set_current_name("untitled.ora");
  }
  if (dialog.run() != Gtk::RESPONSE_ACCEPT) {
    return false;
  }
  path = dialog.get_filename();
  format = format_from_path(path);
  ImageFormat forced = ImageFormat::Unknown;
  auto filter = dialog.get_filter();
  if (filter) {
    const Glib::ustring name = filter->get_name();
    if (name.find("JPEG") != Glib::ustring::npos) {
      forced = ImageFormat::Jpeg;
    } else if (name.find("BMP") != Glib::ustring::npos) {
      forced = ImageFormat::Bmp;
    } else if (name.find("PNG") != Glib::ustring::npos) {
      forced = ImageFormat::Png;
    } else if (name.find("OpenRaster") != Glib::ustring::npos) {
      forced = ImageFormat::Ora;
    }
  }
  if (format == ImageFormat::Unknown) {
    format = forced == ImageFormat::Unknown ? ImageFormat::Ora : forced;
    path += format_extension(format);
  } else if (forced != ImageFormat::Unknown && forced != format) {
    format = forced;
    auto dot = path.find_last_of('.');
    if (dot != std::string::npos) {
      path = path.substr(0, dot);
    }
    path += format_extension(format);
  }
  // Never write a flat image over an existing .ora path.
  if (format != ImageFormat::Ora && format_from_path(path) == ImageFormat::Ora) {
    auto dot = path.find_last_of('.');
    if (dot != std::string::npos) {
      path = path.substr(0, dot);
    }
    path += format_extension(format);
  }
  if (format != ImageFormat::Ora && !document_->path().empty() &&
      format_from_path(document_->path()) == ImageFormat::Ora && path == document_->path()) {
    auto dot = path.find_last_of('.');
    if (dot != std::string::npos) {
      path = path.substr(0, dot);
    }
    path += format_extension(format);
  }
  return true;
}


void MainWindow::copy_selection_to_clipboard() {
  if (document_->selection().empty()) {
    return;
  }
  int w = 0;
  int h = 0;
  std::vector<std::uint8_t> rgba;
  copy_selection_rgba(document_->layers().active_layer(), document_->selection(), document_->width(),
                      document_->height(), w, h, rgba);
  if (w < 1 || h < 1 || rgba.empty()) {
    return;
  }
  auto pixbuf = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, w, h);
  const int dst_stride = pixbuf->get_rowstride();
  std::uint8_t* dst = pixbuf->get_pixels();
  for (int y = 0; y < h; ++y) {
    std::memcpy(dst + static_cast<std::size_t>(y) * dst_stride,
                rgba.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(w) * 4,
                static_cast<std::size_t>(w) * 4);
  }
  Gtk::Clipboard::get()->set_image(pixbuf);
  show_status("Copied");
}

bool MainWindow::paste_from_clipboard() {
  auto pixbuf = Gtk::Clipboard::get()->wait_for_image();
  if (!pixbuf) {
    show_status("Clipboard has no image");
    return false;
  }
  const int w = pixbuf->get_width();
  const int h = pixbuf->get_height();
  if (w < 1 || h < 1) {
    return false;
  }
  auto rgba_buf = pixbuf->add_alpha(false, 0, 0, 0);
  std::vector<std::uint8_t> rgba(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4);
  const int src_stride = rgba_buf->get_rowstride();
  const std::uint8_t* src = rgba_buf->get_pixels();
  const int nch = rgba_buf->get_n_channels();
  for (int y = 0; y < h; ++y) {
    const std::uint8_t* srow = src + static_cast<std::size_t>(y) * src_stride;
    std::uint8_t* drow = rgba.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(w) * 4;
    for (int x = 0; x < w; ++x) {
      const std::uint8_t* p = srow + static_cast<std::size_t>(x) * nch;
      drow[x * 4 + 0] = p[0];
      drow[x * 4 + 1] = p[1];
      drow[x * 4 + 2] = p[2];
      drow[x * 4 + 3] = nch >= 4 ? p[3] : 255;
    }
  }
  document_->paste_floating(0, 0, w, h, std::move(rgba));
  set_active_tool("rect-select");
  show_status("Pasted");
  return true;
}

void MainWindow::action_cut() {
  if (document_->selection().empty()) {
    return;
  }
  copy_selection_to_clipboard();
  document_->delete_selection();
}

void MainWindow::action_copy() {
  copy_selection_to_clipboard();
}

void MainWindow::action_paste() {
  paste_from_clipboard();
}

void MainWindow::action_delete() {
  if (focus_is_editable()) {
    return;
  }
  document_->delete_selection();
}

void MainWindow::action_duplicate() {
  document_->duplicate_selection();
  set_active_tool("rect-select");
}

void MainWindow::action_select_all() {
  document_->select_all();
}

void MainWindow::action_deselect() {
  if (active_tool_ != nullptr && active_tool_->is_stroking()) {
    active_tool_->on_cancel();
  }
  document_->deselect();
}

void MainWindow::action_invert_selection() {
  document_->invert_selection();
}

void MainWindow::action_zoom_fit() {
  canvas_.zoom_fit();
}

void MainWindow::action_toggle_grid() {
  canvas_.set_grid_visible(!canvas_.grid_visible());
  show_status(canvas_.grid_visible() ? "Grid on" : "Grid off");
}

bool MainWindow::warn_size(int width, int height) {
  if (width < 1 || height < 1) {
    return false;
  }
  if (width > kHardMaxSide || height > kHardMaxSide) {
    Gtk::MessageDialog refuse(*this, "Images cannot be larger than 16384 pixels on a side.", false,
                              Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
    refuse.run();
    return false;
  }
  if (width > kSoftMaxSide || height > kSoftMaxSide) {
    Gtk::MessageDialog warn(*this,
                            "This canvas is larger than 8192 pixels on a side and may use a lot of memory.",
                            false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL, true);
    if (warn.run() != Gtk::RESPONSE_OK) {
      return false;
    }
  }
  return true;
}

void MainWindow::commit_buffer_change(const char* name, int new_w, int new_h,
                                      const std::uint8_t* rgba, int stride) {
  const Layer& layer = document_->layers().active_layer();
  auto cmd = LayerBufferCommand::from_buffers(name, document_->width(), document_->height(),
                                              layer.pixels(), layer.stride(), new_w, new_h, rgba,
                                              stride, document_->layers().active_index());
  document_->commit(std::move(cmd));
  canvas_.refresh_size();
  canvas_.invalidate_all();
}

void MainWindow::commit_stack_transform(
    const char* name, int new_w, int new_h,
    const std::function<void(const Layer&, std::vector<std::uint8_t>&, int, int)>& xform) {
  auto old_layers = document_->snapshot_layers();
  std::vector<LayerSnapshot> new_layers;
  new_layers.reserve(old_layers.size());
  for (int i = 0; i < document_->layers().count(); ++i) {
    const Layer& layer = document_->layers().at(i);
    std::vector<std::uint8_t> dest(static_cast<std::size_t>(new_w) * static_cast<std::size_t>(new_h) *
                                   4);
    xform(layer, dest, new_w, new_h);
    LayerSnapshot snap = snapshot_layer_props(layer);
    snap.width = new_w;
    snap.height = new_h;
    snap.offset_x = 0;
    snap.offset_y = 0;
    snap.pixels = std::move(dest);
    new_layers.push_back(std::move(snap));
  }
  auto cmd = std::make_unique<AllLayersBufferCommand>(
      name, document_->width(), document_->height(), document_->layers().active_index(),
      std::move(old_layers), new_w, new_h, document_->layers().active_index(),
      std::move(new_layers));
  document_->commit(std::move(cmd));
  canvas_.refresh_size();
  canvas_.invalidate_all();
}

void MainWindow::action_canvas_size() {
  document_->commit_floating();
  CanvasSizeDialog dialog(*this, document_->width(), document_->height());
  if (dialog.run() != Gtk::RESPONSE_OK) {
    return;
  }
  const int nw = dialog.image_width();
  const int nh = dialog.image_height();
  if (!warn_size(nw, nh)) {
    return;
  }
  const Color fill = dialog.fill_color(document_->background());
  commit_stack_transform("Canvas size", nw, nh, [&](const Layer& layer, std::vector<std::uint8_t>& dest,
                                                    int dw, int dh) {
    resize_canvas(layer.pixels(), layer.width(), layer.height(), layer.stride(), dest.data(), dw, dh,
                  dw * 4, fill);
  });
}

void MainWindow::action_scale() {
  document_->commit_floating();
  ScaleImageDialog dialog(*this, document_->width(), document_->height());
  if (dialog.run() != Gtk::RESPONSE_OK) {
    return;
  }
  const int nw = dialog.image_width();
  const int nh = dialog.image_height();
  if (!warn_size(nw, nh)) {
    return;
  }
  const bool nearest = dialog.nearest();
  commit_stack_transform("Scale", nw, nh, [&](const Layer& layer, std::vector<std::uint8_t>& dest,
                                              int dw, int dh) {
    if (nearest) {
      scale_nearest(layer.pixels(), layer.width(), layer.height(), layer.stride(), dest.data(), dw, dh,
                    dw * 4);
    } else {
      scale_bilinear(layer.pixels(), layer.width(), layer.height(), layer.stride(), dest.data(), dw,
                     dh, dw * 4);
    }
  });
}

void MainWindow::action_crop() {
  document_->commit_floating();
  const Selection& sel = document_->selection();
  if (sel.empty() || sel.inverted()) {
    return;
  }
  Rect r = rect_intersect(sel.bounds(), Rect{0, 0, document_->width(), document_->height()});
  if (r.empty()) {
    return;
  }
  commit_stack_transform("Crop", r.w, r.h, [&](const Layer& layer, std::vector<std::uint8_t>& dest,
                                               int dw, int dh) {
    crop_rect(layer.pixels(), layer.width(), layer.height(), layer.stride(), r, dest.data(), dw * 4);
    (void)dh;
  });
}

void MainWindow::action_autocrop() {
  document_->commit_floating();
  const Layer& layer = document_->layers().active_layer();
  const Rect r = autocrop_bounds(layer.pixels(), layer.width(), layer.height(), layer.stride());
  if (r.empty() || (r.x == 0 && r.y == 0 && r.w == layer.width() && r.h == layer.height())) {
    show_status("Nothing to autocrop");
    return;
  }
  commit_stack_transform("Autocrop", r.w, r.h, [&](const Layer& L, std::vector<std::uint8_t>& dest,
                                                   int dw, int dh) {
    crop_rect(L.pixels(), L.width(), L.height(), L.stride(), r, dest.data(), dw * 4);
    (void)dh;
  });
}

void MainWindow::action_rotate_90() {
  document_->commit_floating();
  const int nw = document_->height();
  const int nh = document_->width();
  commit_stack_transform("Rotate 90", nw, nh, [&](const Layer& layer, std::vector<std::uint8_t>& dest,
                                                  int dw, int dh) {
    rotate_90_cw(layer.pixels(), layer.width(), layer.height(), layer.stride(), dest.data(), dw * 4);
    (void)dh;
  });
}

void MainWindow::action_rotate_180() {
  document_->commit_floating();
  const int w = document_->width();
  const int h = document_->height();
  commit_stack_transform("Rotate 180", w, h, [&](const Layer& layer, std::vector<std::uint8_t>& dest,
                                                 int dw, int dh) {
    dest = copy_layer_pixels(layer);
    rotate_180(dest.data(), layer.width(), layer.height(), layer.width() * 4);
    (void)dw;
    (void)dh;
  });
}

void MainWindow::action_flip_h() {
  document_->commit_floating();
  const int w = document_->width();
  const int h = document_->height();
  commit_stack_transform("Flip horizontal", w, h, [&](const Layer& layer, std::vector<std::uint8_t>& dest,
                                                     int dw, int dh) {
    dest = copy_layer_pixels(layer);
    flip_h(dest.data(), layer.width(), layer.height(), layer.width() * 4);
    (void)dw;
    (void)dh;
  });
}

void MainWindow::action_flip_v() {
  document_->commit_floating();
  const int w = document_->width();
  const int h = document_->height();
  commit_stack_transform("Flip vertical", w, h, [&](const Layer& layer, std::vector<std::uint8_t>& dest,
                                                   int dw, int dh) {
    dest = copy_layer_pixels(layer);
    flip_v(dest.data(), layer.width(), layer.height(), layer.width() * 4);
    (void)dw;
    (void)dh;
  });
}

void MainWindow::action_clear() {
  if (document_->active_locked()) {
    show_status("Layer is locked");
    return;
  }
  document_->commit_floating();
  document_->deselect();
  Layer& layer = document_->layers().active_layer();
  Layer before(layer.width(), layer.height(), Color::transparent(), "before");
  before.copy_from(layer);
  layer.fill(document_->background());
  auto cmd = PixelPatchCommand::from_layers(before, layer, Rect{0, 0, layer.width(), layer.height()},
                                            "Clear", document_->layers().active_index());
  if (cmd && !cmd->empty()) {
    document_->commit(std::move(cmd));
  }
}


void MainWindow::action_layer_new() {
  if (document_->layers().count() >= kSoftMaxLayers) {
    Gtk::MessageDialog warn(*this,
                            "This document has 64 or more layers and may use a lot of memory.",
                            false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL, true);
    if (warn.run() != Gtk::RESPONSE_OK) {
      return;
    }
  }
  document_->add_layer();
  right_dock_.set_current_page(1);
  show_status("Added layer");
}

void MainWindow::action_layer_duplicate() {
  document_->duplicate_layer();
  show_status("Duplicated layer");
}

void MainWindow::action_layer_delete() {
  if (!document_->delete_layer()) {
    show_status("Cannot delete the last layer");
    return;
  }
  show_status("Deleted layer");
}

void MainWindow::action_layer_raise() {
  if (!document_->raise_layer()) {
    return;
  }
  show_status("Raised layer");
}

void MainWindow::action_layer_lower() {
  if (!document_->lower_layer()) {
    return;
  }
  show_status("Lowered layer");
}

void MainWindow::action_layer_merge_down() {
  if (!document_->merge_down()) {
    show_status("Nothing below to merge");
    return;
  }
  show_status("Merged down");
}

void MainWindow::action_layer_flatten() {
  if (document_->layers().count() <= 1) {
    return;
  }
  document_->flatten();
  show_status("Flattened");
}

void MainWindow::action_layer_properties() {
  layers_panel_.show_properties();
}

}  // namespace brushpad
