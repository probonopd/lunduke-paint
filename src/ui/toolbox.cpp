// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/toolbox.hpp"

#include <gtkmm/stylecontext.h>

namespace brushpad {

Toolbox::Toolbox() : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4) {
  set_border_width(4);
  set_size_request(72, -1);

  grid_.set_row_spacing(2);
  grid_.set_column_spacing(2);
  pack_start(grid_, Gtk::PACK_SHRINK);

  wells_.set_size_request(56, 40);
  wells_.set_tooltip_text("Foreground / background. Click a well to choose a color.");
  wells_.add_events(Gdk::BUTTON_PRESS_MASK);
  wells_.signal_draw().connect(sigc::mem_fun(*this, &Toolbox::on_wells_draw));
  wells_.signal_button_press_event().connect(sigc::mem_fun(*this, &Toolbox::on_wells_press));
  pack_start(wells_, Gtk::PACK_SHRINK);

  trans_.set_size_request(56, 18);
  trans_.set_tooltip_text("Transparent: left sets FG, right sets BG (punches alpha)");
  trans_.add_events(Gdk::BUTTON_PRESS_MASK);
  trans_.signal_draw().connect(sigc::mem_fun(*this, &Toolbox::on_trans_draw));
  trans_.signal_button_press_event().connect(sigc::mem_fun(*this, &Toolbox::on_trans_press));
  pack_start(trans_, Gtk::PACK_SHRINK);
}

void Toolbox::add_tool_button(const std::string& id, const std::string& tooltip,
                              const std::string& icon_name) {
  auto* button = Gtk::manage(new Gtk::Button());
  button->set_image_from_icon_name(icon_name, Gtk::ICON_SIZE_SMALL_TOOLBAR);
  button->set_tooltip_text(tooltip);
  button->set_relief(Gtk::RELIEF_NONE);
  button->set_can_focus(false);
  const std::string captured = id;
  button->signal_clicked().connect([this, captured]() {
    if (on_tool_chosen) {
      on_tool_chosen(captured);
    }
  });
  grid_.attach(*button, next_col_, next_row_, 1, 1);
  next_col_ += 1;
  if (next_col_ >= 2) {
    next_col_ = 0;
    next_row_ += 1;
  }
  buttons_.push_back(button);
  ids_.push_back(id);
}

void Toolbox::set_active_tool(const std::string& id) {
  for (std::size_t i = 0; i < buttons_.size(); ++i) {
    if (ids_[i] == id) {
      buttons_[i]->get_style_context()->add_class("suggested-action");
    } else {
      buttons_[i]->get_style_context()->remove_class("suggested-action");
    }
  }
}

void Toolbox::set_colors(Color fg, Color bg) {
  fg_ = fg;
  bg_ = bg;
  wells_.queue_draw();
}

bool Toolbox::on_wells_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
  const int w = wells_.get_allocated_width();
  const int h = wells_.get_allocated_height();
  auto swatch = [&](Color c, int x, int y, int s) {
    cr->set_source_rgb(0.3, 0.3, 0.3);
    cr->rectangle(x, y, s, s);
    cr->stroke();
    if (c.a == 0) {
      cr->set_source_rgb(0.82, 0.82, 0.82);
      cr->rectangle(x + 1, y + 1, s / 2 - 1, s / 2 - 1);
      cr->fill();
      cr->rectangle(x + s / 2, y + s / 2, s / 2 - 1, s / 2 - 1);
      cr->fill();
      cr->set_source_rgb(0.62, 0.62, 0.62);
      cr->rectangle(x + s / 2, y + 1, s / 2 - 1, s / 2 - 1);
      cr->fill();
      cr->rectangle(x + 1, y + s / 2, s / 2 - 1, s / 2 - 1);
      cr->fill();
    } else {
      cr->set_source_rgba(c.r / 255.0, c.g / 255.0, c.b / 255.0, c.a / 255.0);
      cr->rectangle(x + 1, y + 1, s - 2, s - 2);
      cr->fill();
    }
  };
  swatch(bg_, w / 2 - 4, h / 2 - 4, 24);
  swatch(fg_, 6, 4, 24);
  return true;
}

bool Toolbox::on_wells_press(GdkEventButton* event) {
  if (event == nullptr || !on_well_clicked) {
    return false;
  }
  const int w = wells_.get_allocated_width();
  const bool background = event->x > (w * 0.55);
  on_well_clicked(background);
  return true;
}

bool Toolbox::on_trans_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
  const int w = trans_.get_allocated_width();
  const int h = trans_.get_allocated_height();
  const int cell = 6;
  for (int y = 0; y < h; y += cell) {
    for (int x = 0; x < w; x += cell) {
      const bool dark = ((x / cell) + (y / cell)) & 1;
      if (dark) {
        cr->set_source_rgb(0.62, 0.62, 0.62);
      } else {
        cr->set_source_rgb(0.82, 0.82, 0.82);
      }
      cr->rectangle(x, y, cell, cell);
      cr->fill();
    }
  }
  cr->set_source_rgb(0.25, 0.25, 0.25);
  cr->rectangle(0.5, 0.5, w - 1.0, h - 1.0);
  cr->set_line_width(1.0);
  cr->stroke();
  return true;
}

bool Toolbox::on_trans_press(GdkEventButton* event) {
  if (event == nullptr || !on_transparent) {
    return false;
  }
  if (event->button == 1 || event->button == 3) {
    on_transparent(event->button == 3);
    return true;
  }
  return false;
}

}  // namespace brushpad
