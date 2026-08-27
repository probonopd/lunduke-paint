// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_UI_CANVAS_VIEW_HPP
#define BRUSHPAD_UI_CANVAS_VIEW_HPP

#include <gtkmm/drawingarea.h>
#include <sigc++/signal.h>

namespace brushpad {

class CanvasView : public Gtk::DrawingArea {
public:
  CanvasView();

  void reset_blank();

  sigc::signal<void, double, double>& signal_pointer_moved() {
    return signal_pointer_moved_;
  }

  sigc::signal<void>& signal_pointer_left() { return signal_pointer_left_; }

protected:
  bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;
  bool on_motion_notify_event(GdkEventMotion* event) override;
  bool on_leave_notify_event(GdkEventCrossing* event) override;

private:
  sigc::signal<void, double, double> signal_pointer_moved_;
  sigc::signal<void> signal_pointer_left_;
};

}  // namespace brushpad

#endif
