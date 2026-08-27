// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_UI_CANVAS_VIEW_HPP
#define BRUSHPAD_UI_CANVAS_VIEW_HPP

#include "raster/types.hpp"
#include "tools/tool.hpp"

#include <glibmm/refptr.h>
#include <gtkmm/drawingarea.h>
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

  int canvas_width() const;
  int canvas_height() const;

  Color sample_pixel(int canvas_x, int canvas_y) const;

  void widget_to_canvas(double widget_x, double widget_y, double& canvas_x, double& canvas_y) const;

  sigc::signal<void, double, double>& signal_pointer_moved() {
    return signal_pointer_moved_;
  }
  sigc::signal<void>& signal_pointer_left() { return signal_pointer_left_; }
  sigc::signal<void>& signal_view_changed() { return signal_view_changed_; }

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
  Color display_pixel(Color color, int x, int y) const;
  void draw_pixel_grid(const Cairo::RefPtr<Cairo::Context>& cr, int m, int vis_x0, int vis_y0,
                       int vis_x1, int vis_y1);
  void draw_marching_ants(const Cairo::RefPtr<Cairo::Context>& cr, int m);
  void ensure_ants_timer();
  void invalidate_ants();

  Gtk::DrawingArea area_;
  Document* document_{nullptr};
  Tool* tool_{nullptr};
  double zoom_{1.0};
  bool grid_visible_{true};
  int ants_phase_{0};
  sigc::connection ants_timer_;
  bool space_down_{false};
  bool panning_{false};
  double pan_start_x_{0};
  double pan_start_y_{0};
  double pan_hadj_{0};
  double pan_vadj_{0};

  sigc::signal<void, double, double> signal_pointer_moved_;
  sigc::signal<void> signal_pointer_left_;
  sigc::signal<void> signal_view_changed_;
};

}  // namespace brushpad

#endif
