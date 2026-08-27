// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/canvas_view.hpp"

namespace brushpad {

CanvasView::CanvasView() {
  set_can_focus(true);
  set_hexpand(true);
  set_vexpand(true);
  set_size_request(320, 240);
  add_events(Gdk::POINTER_MOTION_MASK | Gdk::LEAVE_NOTIFY_MASK |
             Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK);
}

void CanvasView::reset_blank() {
  queue_draw();
}

bool CanvasView::on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
  cr->set_source_rgb(1.0, 1.0, 1.0);
  cr->paint();
  return true;
}

bool CanvasView::on_motion_notify_event(GdkEventMotion* event) {
  if (event != nullptr) {
    signal_pointer_moved_.emit(event->x, event->y);
  }
  return false;
}

bool CanvasView::on_leave_notify_event(GdkEventCrossing* /*event*/) {
  signal_pointer_left_.emit();
  return false;
}

}  // namespace brushpad
