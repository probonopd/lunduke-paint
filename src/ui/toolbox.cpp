// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/toolbox.hpp"

#include <gdkmm/screen.h>
#include <gtkmm/image.h>
#include <gtkmm/stylecontext.h>

#include <cmath>

namespace brushpad {

namespace {
constexpr int kWellSize = 26;
// Soft ceiling so a theme cannot inflate the strip far past the tool grid;
// natural width is driven by the two equal tool columns + FG/BG captions.
constexpr int kToolboxMaxWidth = 120;
}  // namespace

// Own CSS, loaded once per screen at APPLICATION priority so the selected tool
// looks selected no matter which GTK3 theme is in play.
void Toolbox::ensure_css() {
  static bool loaded = false;
  if (loaded) {
    return;
  }
  auto screen = Gdk::Screen::get_default();
  if (!screen) {
    return;
  }
  auto provider = Gtk::CssProvider::create();
  try {
    provider->load_from_data(toolbox_style::css());
  } catch (const Glib::Error&) {
    return;
  }
  Gtk::StyleContext::add_provider_for_screen(screen, provider,
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  loaded = true;
}

Toolbox::Toolbox() : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 3) {
  ensure_css();
  set_border_width(2);
  set_halign(Gtk::ALIGN_START);
  // Width tracks the tool grid (plus FG/BG captions). No large fixed pad.
  set_size_request(-1, -1);

  grid_.set_row_spacing(2);
  grid_.set_column_spacing(2);
  grid_.set_column_homogeneous(true);
  // Fill the strip so equal columns match caption-row width (no empty side pad).
  grid_.set_hexpand(true);
  grid_.set_halign(Gtk::ALIGN_FILL);
  pack_start(grid_, Gtk::PACK_SHRINK);

  auto setup_well = [](Gtk::DrawingArea& well, const char* tip) {
    well.set_size_request(kWellSize, kWellSize);
    well.set_valign(Gtk::ALIGN_CENTER);
    well.set_tooltip_text(tip);
    well.add_events(Gdk::BUTTON_PRESS_MASK);
  };
  setup_well(fg_well_, "Foreground color");
  setup_well(bg_well_, "Background color");
  fg_well_.set_halign(Gtk::ALIGN_START);
  bg_well_.set_halign(Gtk::ALIGN_END);
  fg_well_.signal_draw().connect(sigc::mem_fun(*this, &Toolbox::on_fg_well_draw));
  bg_well_.signal_draw().connect(sigc::mem_fun(*this, &Toolbox::on_bg_well_draw));
  fg_well_.signal_button_press_event().connect(sigc::mem_fun(*this, &Toolbox::on_fg_well_press));
  bg_well_.signal_button_press_event().connect(sigc::mem_fun(*this, &Toolbox::on_bg_well_press));

  fg_label_.set_valign(Gtk::ALIGN_CENTER);
  bg_label_.set_valign(Gtk::ALIGN_CENTER);
  fg_label_.set_halign(Gtk::ALIGN_START);
  bg_label_.set_halign(Gtk::ALIGN_END);
  fg_label_.set_xalign(0.0);
  bg_label_.set_xalign(1.0);
  fg_label_.set_line_wrap(false);
  bg_label_.set_line_wrap(false);
  fg_label_.set_tooltip_text("Foreground color");
  bg_label_.set_tooltip_text("Background color");
  fg_label_.get_style_context()->add_class(toolbox_style::caption_class());
  bg_label_.get_style_context()->add_class(toolbox_style::caption_class());

  // Foreground label immediately to the right of the FG well, vertically centered.
  fg_row_.set_halign(Gtk::ALIGN_FILL);
  fg_row_.set_hexpand(true);
  fg_row_.set_valign(Gtk::ALIGN_CENTER);
  fg_row_.pack_start(fg_well_, Gtk::PACK_SHRINK);
  fg_row_.pack_start(fg_label_, Gtk::PACK_SHRINK);

  // Background label immediately to the left of the BG well; well is
  // right-justified and dropped a bit so the two captions cannot overlap.
  bg_row_.set_halign(Gtk::ALIGN_FILL);
  bg_row_.set_hexpand(true);
  bg_row_.set_valign(Gtk::ALIGN_CENTER);
  bg_row_.set_margin_top(8);
  bg_row_.pack_start(bg_label_, Gtk::PACK_EXPAND_WIDGET);
  bg_row_.pack_end(bg_well_, Gtk::PACK_SHRINK);

  pack_start(fg_row_, Gtk::PACK_SHRINK);
  pack_start(bg_row_, Gtk::PACK_SHRINK);

  // Height only — width follows the toolbox/grid, not a fixed pad.
  trans_.set_size_request(-1, 18);
  trans_.set_hexpand(true);
  trans_.set_tooltip_text("Transparent: left sets FG, right sets BG (punches alpha)");
  trans_.add_events(Gdk::BUTTON_PRESS_MASK);
  trans_.signal_draw().connect(sigc::mem_fun(*this, &Toolbox::on_trans_draw));
  trans_.signal_button_press_event().connect(sigc::mem_fun(*this, &Toolbox::on_trans_press));
  pack_start(trans_, Gtk::PACK_SHRINK);

}

void Toolbox::add_tool_button(const std::string& id, const std::string& tooltip,
                              const std::string& icon_name) {
  auto* button = Gtk::manage(new Gtk::Button());
  const std::string resource =
      "/org/lunduke/LundukePaint/icons/scalable/actions/" + icon_name + ".svg";
  auto* image = Gtk::manage(new Gtk::Image());
  image->set_from_resource(resource);
  image->set_pixel_size(18);
  button->set_image(*image);
  button->set_tooltip_text(tooltip);
  button->set_relief(Gtk::RELIEF_NONE);
  button->set_can_focus(false);
  button->set_hexpand(true);
  button->set_halign(Gtk::ALIGN_FILL);
  button->get_style_context()->add_class(toolbox_style::button_class());
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
  selection_.add(id);
  apply_selection_style();
}

void Toolbox::set_active_tool(const std::string& id) {
  selection_.select(id);
  apply_selection_style();
}

void Toolbox::apply_selection_style() {
  const std::vector<std::string>& ids = selection_.ids();
  for (std::size_t i = 0; i < buttons_.size() && i < ids.size(); ++i) {
    auto context = buttons_[i]->get_style_context();
    if (selection_.is_selected(ids[i])) {
      context->add_class(toolbox_style::selected_class());
      // Keep the theme hint too, for themes that style it nicely.
      context->add_class("suggested-action");
    } else {
      context->remove_class(toolbox_style::selected_class());
      context->remove_class("suggested-action");
    }
  }
}

bool Toolbox::tool_button_selected(const std::string& id) const {
  const int index = tool_index(selection_.ids(), id);
  if (index < 0 || index >= static_cast<int>(buttons_.size())) {
    return false;
  }
  return buttons_[static_cast<std::size_t>(index)]->get_style_context()->has_class(
      toolbox_style::selected_class());
}

bool Toolbox::tool_columns_homogeneous() const { return grid_.get_column_homogeneous(); }

bool Toolbox::tool_columns_equal_width() const {
  if (buttons_.size() < 2) {
    return false;
  }
  const int w0 = buttons_[0]->get_allocated_width();
  const int w1 = buttons_[1]->get_allocated_width();
  return w0 > 8 && std::abs(w0 - w1) <= 1;
}

bool Toolbox::width_tracks_tool_grid() const {
  const int box_w = get_allocated_width();
  const int grid_w = grid_.get_allocated_width();
  if (box_w < 1 || grid_w < 1) {
    return false;
  }
  // Border plus a little theme chrome — not a large fixed pad like the old 156px.
  const int pad = box_w - grid_w;
  return pad >= 0 && pad <= 24 && box_w <= kToolboxMaxWidth + 12;
}

bool Toolbox::child_origin(const Gtk::Widget& child, int& x, int& y) const {
  x = 0;
  y = 0;
  return const_cast<Gtk::Widget&>(child).translate_coordinates(const_cast<Toolbox&>(*this), 0, 0, x,
                                                               y);
}

bool Toolbox::fg_label_right_of_well() const {
  int lx = 0;
  int ly = 0;
  int wx = 0;
  int wy = 0;
  if (!child_origin(fg_label_, lx, ly) || !child_origin(fg_well_, wx, wy)) {
    return false;
  }
  const int well_right = wx + fg_well_.get_allocated_width();
  const int label_cx = ly + fg_label_.get_allocated_height() / 2;
  const int well_cx = wy + fg_well_.get_allocated_height() / 2;
  return lx >= well_right - 1 && std::abs(label_cx - well_cx) <= 10;
}

bool Toolbox::bg_label_left_of_well() const {
  int lx = 0;
  int ly = 0;
  int wx = 0;
  int wy = 0;
  if (!child_origin(bg_label_, lx, ly) || !child_origin(bg_well_, wx, wy)) {
    return false;
  }
  const int label_right = lx + bg_label_.get_allocated_width();
  const int label_cx = ly + bg_label_.get_allocated_height() / 2;
  const int well_cx = wy + bg_well_.get_allocated_height() / 2;
  // Immediately left of the well (small gap for the row spacing).
  return label_right <= wx + 2 && label_right >= wx - 16 && std::abs(label_cx - well_cx) <= 10;
}

bool Toolbox::bg_well_right_justified() const {
  int wx = 0;
  int wy = 0;
  if (!child_origin(bg_well_, wx, wy)) {
    return false;
  }
  const int well_right = wx + bg_well_.get_allocated_width();
  const int box_right = get_allocated_width();
  return well_right >= box_right - 16;
}

bool Toolbox::bg_well_below_fg() const {
  int fgy = 0;
  int bgy = 0;
  int fgx = 0;
  int bgx = 0;
  if (!child_origin(fg_well_, fgx, fgy) || !child_origin(bg_well_, bgx, bgy)) {
    return false;
  }
  return bgy >= fgy + fg_well_.get_allocated_height() - 2;
}

void Toolbox::set_colors(Color fg, Color bg) {
  fg_ = fg;
  bg_ = bg;
  fg_well_.queue_draw();
  bg_well_.queue_draw();
}

void Toolbox::draw_swatch(const Cairo::RefPtr<Cairo::Context>& cr, Gtk::DrawingArea& area, Color c) {
  const int w = area.get_allocated_width();
  const int h = area.get_allocated_height();
  const int s = std::min(w, h);
  const int x = (w - s) / 2;
  const int y = (h - s) / 2;
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
}

bool Toolbox::on_fg_well_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
  draw_swatch(cr, fg_well_, fg_);
  return true;
}

bool Toolbox::on_bg_well_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
  draw_swatch(cr, bg_well_, bg_);
  return true;
}

bool Toolbox::on_fg_well_press(GdkEventButton* event) {
  if (event == nullptr || !on_well_clicked) {
    return false;
  }
  on_well_clicked(false);
  return true;
}

bool Toolbox::on_bg_well_press(GdkEventButton* event) {
  if (event == nullptr || !on_well_clicked) {
    return false;
  }
  on_well_clicked(true);
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
