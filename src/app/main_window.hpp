// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_APP_MAIN_WINDOW_HPP
#define BRUSHPAD_APP_MAIN_WINDOW_HPP

#include "doc/document.hpp"
#include "io/image_io.hpp"
#include "tools/tool.hpp"
#include "ui/canvas_view.hpp"
#include "ui/colors_panel.hpp"
#include "ui/history_panel.hpp"
#include "ui/layers_panel.hpp"
#include "ui/status_bar.hpp"
#include "ui/tool_options_bar.hpp"
#include "ui/toolbox.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <giomm/menumodel.h>
#include <giomm/simpleaction.h>
#include <glibmm/ustring.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/box.h>
#include <gtkmm/notebook.h>

namespace brushpad {

class MainWindow : public Gtk::ApplicationWindow, public ToolHost {
public:
  MainWindow();
  ~MainWindow() override;

  void reset_canvas();
  void new_document(int width, int height, Color background);
  void show_status(const Glib::ustring& message);
  void action_new();
  void action_open();
  void action_save();
  void action_save_as();
  bool confirm_close();
  Document& document() override { return *document_; }
  Document* document_ptr() { return document_.get(); }
  CanvasView& canvas() { return canvas_; }

  void set_active_tool(const std::string& id);
  Tool* active_tool() const { return active_tool_; }

  int stroke_size() const override { return stroke_size_; }
  void set_stroke_size(int size) override;
  bool brush_antialias() const override { return brush_aa_; }
  void set_brush_antialias(bool enabled) override;
  int fill_tolerance() const override { return fill_tolerance_; }
  void set_fill_tolerance(int tolerance) override;
  void invalidate_canvas(Rect rect) override;
  void return_to_previous_tool() override;
  Color sample_canvas(int x, int y) const override;
  void show_status_hint(const char* message) override;

  void update_chrome();
  void on_undo();
  void on_redo();
  void action_cut();
  void action_copy();
  void action_paste();
  void action_delete();
  void action_duplicate();
  void action_select_all();
  void action_deselect();
  void action_invert_selection();
  void action_canvas_size();
  void action_scale();
  void action_crop();
  void action_autocrop();
  void action_rotate_90();
  void action_rotate_180();
  void action_flip_h();
  void action_flip_v();
  void action_clear();
  void action_zoom_fit();
  void action_toggle_grid();
  void action_layer_new();
  void action_layer_duplicate();
  void action_layer_delete();
  void action_layer_raise();
  void action_layer_lower();
  void action_layer_merge_down();
  void action_layer_flatten();
  void action_layer_properties();

private:
  void build_ui();
  void build_toolbar();
  Glib::RefPtr<Gio::MenuModel> load_menubar_model();
  void on_toggle_right_dock();
  void bind_document();
  void choose_color(bool background);
  bool on_key_press(GdkEventKey* event);
  bool on_key_release(GdkEventKey* event);
  bool focus_is_editable() const;
  void update_title();
  bool confirm_lose_changes();
  bool save_to_path(const std::string& path, ImageFormat format);
  std::string choose_open_path();
  bool choose_save_path(std::string& path, ImageFormat& format);
  bool layer_has_transparency() const;
  bool on_delete_event(GdkEventAny* event) override;
  void commit_buffer_change(const char* name, int new_w, int new_h, const std::uint8_t* rgba,
                            int stride);
  void commit_stack_transform(
      const char* name, int new_w, int new_h,
      const std::function<void(const Layer&, std::vector<std::uint8_t>&, int, int)>& xform);
  bool warn_size(int width, int height);
  void copy_selection_to_clipboard();
  bool paste_from_clipboard();

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
  std::unique_ptr<Document> document_;
  std::vector<std::unique_ptr<Tool>> tools_;
  Tool* active_tool_{nullptr};
  Tool* previous_tool_{nullptr};
  int stroke_size_{1};
  bool brush_aa_{true};
  int fill_tolerance_{0};
  Glib::RefPtr<Gio::SimpleAction> undo_action_;
  Glib::RefPtr<Gio::SimpleAction> redo_action_;
  Glib::RefPtr<Gio::SimpleAction> cut_action_;
  Glib::RefPtr<Gio::SimpleAction> copy_action_;
  Glib::RefPtr<Gio::SimpleAction> delete_action_;
  Glib::RefPtr<Gio::SimpleAction> duplicate_action_;
  Glib::RefPtr<Gio::SimpleAction> deselect_action_;
  Glib::RefPtr<Gio::SimpleAction> invert_action_;
  Glib::RefPtr<Gio::SimpleAction> crop_action_;
  Glib::RefPtr<Gio::SimpleAction> layer_delete_action_;
  Glib::RefPtr<Gio::SimpleAction> layer_raise_action_;
  Glib::RefPtr<Gio::SimpleAction> layer_lower_action_;
  Glib::RefPtr<Gio::SimpleAction> layer_merge_action_;
  Glib::RefPtr<Gio::SimpleAction> layer_flatten_action_;
};

}  // namespace brushpad

#endif
