// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_UI_CANVAS_VIEW_HPP
#define BRUSHPAD_UI_CANVAS_VIEW_HPP

#include "raster/types.hpp"
#include "tools/tool.hpp"

#include <glibmm/refptr.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/layout.h>
#include <gtkmm/scrolledwindow.h>
#include <sigc++/connection.h>
#include <sigc++/signal.h>

namespace brushpad {

class Document;
class Layer;
class Tool;

class CanvasView : public Gtk::ScrolledWindow {
public:
  CanvasView();

  void set_document(Document* document);
  void set_tool(Tool* tool);
  void set_space_down(bool down);

  void reset_blank();
  void invalidate_rect(Rect rect);
  void invalidate_all();
  void refresh_size();

  double zoom() const { return zoom_; }
  void set_zoom(double zoom);
  void zoom_in();
  void zoom_out();
  void zoom_in_at(double widget_x, double widget_y);
  void zoom_out_at(double widget_x, double widget_y);
  void zoom_to(double zoom, double widget_x, double widget_y);
  void zoom_fit();

  bool grid_visible() const { return grid_visible_; }
  void set_grid_visible(bool visible);
  void set_checker_colors(Color light, Color dark);
  void set_grid_threshold(int percent);

  int canvas_width() const;
  int canvas_height() const;

  Color sample_pixel(int canvas_x, int canvas_y) const;

  void widget_to_canvas(double widget_x, double widget_y, double& canvas_x, double& canvas_y) const;
  bool canvas_to_screen(int canvas_x, int canvas_y, int& screen_x, int& screen_y) const;
  bool last_pointer(int& canvas_x, int& canvas_y) const;
  void viewport_center_canvas(int& canvas_x, int& canvas_y) const;
  void apply_zoom(double zoom);

  sigc::signal<void, double, double>& signal_pointer_moved() {
    return signal_pointer_moved_;
  }
  sigc::signal<void>& signal_pointer_left() { return signal_pointer_left_; }
  sigc::signal<void>& signal_view_changed() { return signal_view_changed_; }

protected:
  void on_size_allocate(Gtk::Allocation& allocation) override;

private:
  bool on_area_draw(const Cairo::RefPtr<Cairo::Context>& cr);
  bool on_area_motion(GdkEventMotion* event);
  bool on_area_leave(GdkEventCrossing* event);
  bool on_area_button_press(GdkEventButton* event);
  bool on_area_button_release(GdkEventButton* event);
  bool on_area_scroll(GdkEventScroll* event);

  void update_area_size();
  unsigned modifiers_from_state(guint state) const;
  CanvasEvent make_event(double widget_x, double widget_y, unsigned button, guint state) const;
  void begin_pan(double widget_x, double widget_y);
  void update_pan(double widget_x, double widget_y);
  double snapped_zoom(double zoom) const;
  void visible_center(double& x, double& y) const;
  int margin() const { return 24; }
  int content_pixel_width() const;
  int content_pixel_height() const;
  int origin_x() const;
  int origin_y() const;
  Color display_pixel(Color color, int x, int y) const;
  void draw_pixel_grid(const Cairo::RefPtr<Cairo::Context>& cr, int ox, int oy, int vis_x0,
                       int vis_y0, int vis_x1, int vis_y1);
  void draw_marching_ants(const Cairo::RefPtr<Cairo::Context>& cr, int ox, int oy);
  void ensure_ants_timer();
  void invalidate_ants();

  Gtk::Layout layout_;
  Gtk::DrawingArea area_;
  Document* document_{nullptr};
  Tool* tool_{nullptr};
  double zoom_{1.0};
  bool grid_visible_{true};
  Color checker_light_{209, 209, 209, 255};
  Color checker_dark_{158, 158, 158, 255};
  int grid_threshold_{400};
  int ants_phase_{0};
  sigc::connection ants_timer_;
  bool space_down_{false};
  bool panning_{false};
  double pan_start_x_{0};
  double pan_start_y_{0};
  double pan_hadj_{0};
  double pan_vadj_{0};
  bool updating_size_{false};

  sigc::signal<void, double, double> signal_pointer_moved_;
  sigc::signal<void> signal_pointer_left_;
  sigc::signal<void> signal_view_changed_;
  bool has_pointer_{false};
  double last_cx_{0};
  double last_cy_{0};
};

}  // namespace brushpad

#endif
