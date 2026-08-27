// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/canvas_view.hpp"

#include "doc/document.hpp"

#include <gtkmm/adjustment.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace brushpad {
namespace {

constexpr double kZoomMin = 0.125;
constexpr double kZoomMax = 16.0;

void write_argb32(std::uint8_t* dst, Color c) {
  const int a = c.a;
  dst[0] = static_cast<std::uint8_t>((static_cast<int>(c.b) * a + 127) / 255);
  dst[1] = static_cast<std::uint8_t>((static_cast<int>(c.g) * a + 127) / 255);
  dst[2] = static_cast<std::uint8_t>((static_cast<int>(c.r) * a + 127) / 255);
  dst[3] = c.a;
}

void draw_checker(const Cairo::RefPtr<Cairo::Context>& cr, int x, int y, int w, int h) {
  const int cell = 8;
  const int x0 = x;
  const int y0 = y;
  const int x1 = x + w;
  const int y1 = y + h;
  for (int cy = y0; cy < y1; cy += cell) {
    for (int cx = x0; cx < x1; cx += cell) {
      const int tx = ((cx - x0) / cell) + ((cy - y0) / cell);
      if ((tx & 1) == 0) {
        cr->set_source_rgb(0.82, 0.82, 0.82);
      } else {
        cr->set_source_rgb(0.62, 0.62, 0.62);
      }
      const int cw = std::min(cell, x1 - cx);
      const int ch = std::min(cell, y1 - cy);
      cr->rectangle(cx, cy, cw, ch);
      cr->fill();
    }
  }
}

}  // namespace

CanvasView::CanvasView() {
  set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  set_hexpand(true);
  set_vexpand(true);
  set_shadow_type(Gtk::SHADOW_IN);

  area_.set_can_focus(true);
  area_.set_hexpand(true);
  area_.set_vexpand(true);
  area_.add_events(Gdk::POINTER_MOTION_MASK | Gdk::LEAVE_NOTIFY_MASK | Gdk::BUTTON_PRESS_MASK |
                   Gdk::BUTTON_RELEASE_MASK | Gdk::SCROLL_MASK | Gdk::SMOOTH_SCROLL_MASK |
                   Gdk::BUTTON_MOTION_MASK);

  area_.signal_draw().connect(sigc::mem_fun(*this, &CanvasView::on_area_draw), false);
  area_.signal_motion_notify_event().connect(sigc::mem_fun(*this, &CanvasView::on_area_motion),
                                             false);
  area_.signal_leave_notify_event().connect(sigc::mem_fun(*this, &CanvasView::on_area_leave),
                                            false);
  area_.signal_button_press_event().connect(sigc::mem_fun(*this, &CanvasView::on_area_button_press),
                                            false);
  area_.signal_button_release_event().connect(
      sigc::mem_fun(*this, &CanvasView::on_area_button_release), false);
  area_.signal_scroll_event().connect(sigc::mem_fun(*this, &CanvasView::on_area_scroll), false);

  add(area_);
  update_area_size();
}

void CanvasView::set_document(Document* document) {
  document_ = document;
  update_area_size();
  queue_draw();
  signal_view_changed_.emit();
}

void CanvasView::set_tool(Tool* tool) {
  tool_ = tool;
}

void CanvasView::set_space_down(bool down) {
  space_down_ = down;
}

void CanvasView::reset_blank() {
  update_area_size();
  invalidate_all();
}

void CanvasView::invalidate_rect(Rect rect) {
  if (rect.empty()) {
    invalidate_all();
    return;
  }
  const int m = margin();
  const double z = zoom_;
  const int x = static_cast<int>(std::floor(m + rect.x * z)) - 2;
  const int y = static_cast<int>(std::floor(m + rect.y * z)) - 2;
  const int w = static_cast<int>(std::ceil(rect.w * z)) + 4;
  const int h = static_cast<int>(std::ceil(rect.h * z)) + 4;
  area_.queue_draw_area(x, y, w, h);
}

void CanvasView::invalidate_all() {
  area_.queue_draw();
}

void CanvasView::visible_center(double& x, double& y) const {
  const auto hadj = get_hadjustment();
  const auto vadj = get_vadjustment();
  x = hadj ? hadj->get_value() + hadj->get_page_size() * 0.5 : area_.get_allocated_width() * 0.5;
  y = vadj ? vadj->get_value() + vadj->get_page_size() * 0.5 : area_.get_allocated_height() * 0.5;
}

void CanvasView::set_zoom(double zoom) {
  double x = 0;
  double y = 0;
  visible_center(x, y);
  zoom_to(zoom, x, y);
}

void CanvasView::zoom_in() {
  double x = 0;
  double y = 0;
  visible_center(x, y);
  zoom_in_at(x, y);
}

void CanvasView::zoom_out() {
  double x = 0;
  double y = 0;
  visible_center(x, y);
  zoom_out_at(x, y);
}

void CanvasView::zoom_in_at(double widget_x, double widget_y) {
  zoom_to(zoom_ * 2.0, widget_x, widget_y);
}

void CanvasView::zoom_out_at(double widget_x, double widget_y) {
  zoom_to(zoom_ * 0.5, widget_x, widget_y);
}

double CanvasView::snapped_zoom(double zoom) const {
  zoom = std::clamp(zoom, kZoomMin, kZoomMax);
  static const double steps[] = {0.125, 0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0};
  double best = steps[0];
  double best_d = std::abs(zoom - best);
  for (double step : steps) {
    const double d = std::abs(zoom - step);
    if (d < best_d) {
      best = step;
      best_d = d;
    }
  }
  return best;
}

void CanvasView::zoom_to(double zoom, double widget_x, double widget_y) {
  zoom = snapped_zoom(zoom);
  if (std::abs(zoom - zoom_) < 1e-6) {
    return;
  }

  double canvas_x = 0;
  double canvas_y = 0;
  widget_to_canvas(widget_x, widget_y, canvas_x, canvas_y);

  const auto hadj = get_hadjustment();
  const auto vadj = get_vadjustment();

  zoom_ = zoom;
  update_area_size();

  // Keep the canvas point under the pointer after the size change.
  if (hadj) {
    const double new_widget_x = margin() + canvas_x * zoom_;
    hadj->set_value(new_widget_x - widget_x);
  }
  if (vadj) {
    const double new_widget_y = margin() + canvas_y * zoom_;
    vadj->set_value(new_widget_y - widget_y);
  }

  invalidate_all();
  signal_view_changed_.emit();
}

int CanvasView::canvas_width() const {
  return document_ != nullptr ? document_->width() : kDefaultWidth;
}

int CanvasView::canvas_height() const {
  return document_ != nullptr ? document_->height() : kDefaultHeight;
}

Color CanvasView::sample_pixel(int canvas_x, int canvas_y) const {
  if (document_ == nullptr) {
    return Color::transparent();
  }
  const Layer& layer = (tool_ != nullptr && tool_->is_stroking())
                           ? document_->layers().tool_layer()
                           : document_->layers().active_layer();
  return layer.pixel(canvas_x, canvas_y);
}

void CanvasView::widget_to_canvas(double widget_x, double widget_y, double& canvas_x,
                                 double& canvas_y) const {
  canvas_x = (widget_x - margin()) / zoom_;
  canvas_y = (widget_y - margin()) / zoom_;
}

void CanvasView::update_area_size() {
  const int w = static_cast<int>(std::ceil(canvas_width() * zoom_)) + margin() * 2;
  const int h = static_cast<int>(std::ceil(canvas_height() * zoom_)) + margin() * 2;
  area_.set_size_request(w, h);
}

unsigned CanvasView::modifiers_from_state(guint state) const {
  unsigned mods = 0;
  if (state & GDK_SHIFT_MASK) {
    mods |= Modifier::Shift;
  }
  if (state & GDK_CONTROL_MASK) {
    mods |= Modifier::Ctrl;
  }
  if (state & GDK_MOD1_MASK) {
    mods |= Modifier::Alt;
  }
  return mods;
}

CanvasEvent CanvasView::make_event(double widget_x, double widget_y, unsigned button,
                                   guint state) const {
  CanvasEvent event;
  widget_to_canvas(widget_x, widget_y, event.x, event.y);
  event.button = button;
  event.modifiers = modifiers_from_state(state);
  return event;
}

void CanvasView::begin_pan(double widget_x, double widget_y) {
  panning_ = true;
  pan_start_x_ = widget_x;
  pan_start_y_ = widget_y;
  const auto hadj = get_hadjustment();
  const auto vadj = get_vadjustment();
  pan_hadj_ = hadj ? hadj->get_value() : 0.0;
  pan_vadj_ = vadj ? vadj->get_value() : 0.0;
}

void CanvasView::update_pan(double widget_x, double widget_y) {
  const auto hadj = get_hadjustment();
  const auto vadj = get_vadjustment();
  if (hadj) {
    hadj->set_value(pan_hadj_ - (widget_x - pan_start_x_));
  }
  if (vadj) {
    vadj->set_value(pan_vadj_ - (widget_y - pan_start_y_));
  }
}

bool CanvasView::on_area_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
  const Gtk::Allocation alloc = area_.get_allocation();
  auto context = area_.get_style_context();
  Gdk::RGBA bg;
  if (!context->lookup_color("theme_bg_color", bg)) {
    bg.set_rgba(0.85, 0.85, 0.85, 1.0);
  }
  cr->set_source_rgb(bg.get_red(), bg.get_green(), bg.get_blue());
  cr->paint();

  const int m = margin();
  const int cw = canvas_width();
  const int ch = canvas_height();
  const int dw = static_cast<int>(std::ceil(cw * zoom_));
  const int dh = static_cast<int>(std::ceil(ch * zoom_));

  cr->save();
  cr->rectangle(m, m, dw, dh);
  cr->clip();
  draw_checker(cr, m, m, dw, dh);

  if (document_ == nullptr) {
    cr->restore();
    return true;
  }

  const Layer& layer = (tool_ != nullptr && tool_->is_stroking())
                           ? document_->layers().tool_layer()
                           : document_->layers().active_layer();

  double clip_x1 = 0;
  double clip_y1 = 0;
  double clip_x2 = 0;
  double clip_y2 = 0;
  cr->get_clip_extents(clip_x1, clip_y1, clip_x2, clip_y2);

  const int vis_x0 = std::max(0, static_cast<int>(std::floor((clip_x1 - m) / zoom_)));
  const int vis_y0 = std::max(0, static_cast<int>(std::floor((clip_y1 - m) / zoom_)));
  const int vis_x1 = std::min(cw, static_cast<int>(std::ceil((clip_x2 - m) / zoom_)));
  const int vis_y1 = std::min(ch, static_cast<int>(std::ceil((clip_y2 - m) / zoom_)));
  if (vis_x1 <= vis_x0 || vis_y1 <= vis_y0) {
    cr->restore();
    return true;
  }

  const bool integer_up = zoom_ >= 1.0 - 1e-9;
  if (integer_up) {
    const int dest_x = static_cast<int>(std::floor(m + vis_x0 * zoom_));
    const int dest_y = static_cast<int>(std::floor(m + vis_y0 * zoom_));
    const int dest_w = static_cast<int>(std::ceil((vis_x1 - vis_x0) * zoom_));
    const int dest_h = static_cast<int>(std::ceil((vis_y1 - vis_y0) * zoom_));
    if (dest_w > 0 && dest_h > 0) {
      auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, dest_w, dest_h);
      std::uint8_t* dst = surface->get_data();
      const int dst_stride = surface->get_stride();
      const std::uint8_t* src = layer.pixels();
      const int src_stride = layer.stride();
      const int iz = std::max(1, static_cast<int>(std::lround(zoom_)));
      const bool exact = std::abs(zoom_ - static_cast<double>(iz)) < 1e-6;
      for (int dy = 0; dy < dest_h; ++dy) {
        int sy = vis_y0;
        if (exact) {
          sy = vis_y0 + dy / iz;
        } else {
          sy = vis_y0 + static_cast<int>(std::floor(dy / zoom_));
        }
        if (sy >= ch) {
          sy = ch - 1;
        }
        std::uint8_t* drow = dst + static_cast<std::size_t>(dy) * dst_stride;
        const std::uint8_t* srow = src + static_cast<std::size_t>(sy) * src_stride;
        for (int dx = 0; dx < dest_w; ++dx) {
          int sx = vis_x0;
          if (exact) {
            sx = vis_x0 + dx / iz;
          } else {
            sx = vis_x0 + static_cast<int>(std::floor(dx / zoom_));
          }
          if (sx >= cw) {
            sx = cw - 1;
          }
          const std::uint8_t* p = srow + static_cast<std::size_t>(sx) * 4;
          write_argb32(drow + static_cast<std::size_t>(dx) * 4, Color{p[0], p[1], p[2], p[3]});
        }
      }
      surface->mark_dirty();
      cr->set_source(surface, dest_x, dest_y);
      cr->paint();
    }
  } else {
    const int sw = vis_x1 - vis_x0;
    const int sh = vis_y1 - vis_y0;
    auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, sw, sh);
    std::uint8_t* dst = surface->get_data();
    const int dst_stride = surface->get_stride();
    const std::uint8_t* src = layer.pixels();
    const int src_stride = layer.stride();
    for (int y = 0; y < sh; ++y) {
      std::uint8_t* drow = dst + static_cast<std::size_t>(y) * dst_stride;
      const std::uint8_t* srow =
          src + static_cast<std::size_t>(vis_y0 + y) * src_stride + static_cast<std::size_t>(vis_x0) * 4;
      for (int x = 0; x < sw; ++x) {
        const std::uint8_t* p = srow + static_cast<std::size_t>(x) * 4;
        write_argb32(drow + static_cast<std::size_t>(x) * 4, Color{p[0], p[1], p[2], p[3]});
      }
    }
    surface->mark_dirty();
    cr->save();
    cr->translate(m + vis_x0 * zoom_, m + vis_y0 * zoom_);
    cr->scale(zoom_, zoom_);
    cr->set_source(surface, 0, 0);
    cr->paint();
    cr->restore();
  }

  cr->restore();
  (void)alloc;
  return true;
}

bool CanvasView::on_area_motion(GdkEventMotion* event) {
  if (event == nullptr) {
    return false;
  }
  double cx = 0;
  double cy = 0;
  widget_to_canvas(event->x, event->y, cx, cy);
  signal_pointer_moved_.emit(cx, cy);

  if (panning_) {
    update_pan(event->x, event->y);
    return true;
  }
  if (tool_ != nullptr && tool_->is_stroking()) {
    tool_->on_motion(make_event(event->x, event->y, 0, event->state));
    return true;
  }
  return false;
}

bool CanvasView::on_area_leave(GdkEventCrossing* /*event*/) {
  signal_pointer_left_.emit();
  return false;
}

bool CanvasView::on_area_button_press(GdkEventButton* event) {
  if (event == nullptr) {
    return false;
  }
  area_.grab_focus();
  if (event->button == 2 || (event->button == 1 && space_down_)) {
    begin_pan(event->x, event->y);
    return true;
  }
  if (tool_ != nullptr && (event->button == 1 || event->button == 3)) {
    tool_->on_press(make_event(event->x, event->y, event->button, event->state));
    return true;
  }
  return false;
}

bool CanvasView::on_area_button_release(GdkEventButton* event) {
  if (event == nullptr) {
    return false;
  }
  if (panning_ && (event->button == 2 || event->button == 1)) {
    panning_ = false;
    return true;
  }
  if (tool_ != nullptr && (event->button == 1 || event->button == 3)) {
    tool_->on_release(make_event(event->x, event->y, event->button, event->state));
    return true;
  }
  return false;
}

bool CanvasView::on_area_scroll(GdkEventScroll* event) {
  if (event == nullptr) {
    return false;
  }
  if ((event->state & GDK_CONTROL_MASK) == 0) {
    return false;
  }
  bool zoom_in = false;
  if (event->direction == GDK_SCROLL_UP) {
    zoom_in = true;
  } else if (event->direction == GDK_SCROLL_DOWN) {
    zoom_in = false;
  } else if (event->direction == GDK_SCROLL_SMOOTH) {
    if (event->delta_y < 0) {
      zoom_in = true;
    } else if (event->delta_y > 0) {
      zoom_in = false;
    } else {
      return true;
    }
  } else {
    return false;
  }
  if (zoom_in) {
    zoom_in_at(event->x, event->y);
  } else {
    zoom_out_at(event->x, event->y);
  }
  return true;
}

}  // namespace brushpad
