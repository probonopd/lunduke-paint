// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/colors_panel.hpp"

namespace lundukepaint {
namespace {

// Classic 48-color Paint / KolourPaint-style palette, last cell transparent.
const Color kDefaultPalette[48] = {
    {0, 0, 0, 255},       {128, 128, 128, 255}, {128, 0, 0, 255},     {128, 128, 0, 255},
    {0, 128, 0, 255},     {0, 128, 128, 255},   {0, 0, 128, 255},     {128, 0, 128, 255},
    {128, 128, 64, 255},  {0, 64, 64, 255},     {0, 128, 255, 255},   {0, 64, 128, 255},
    {128, 0, 255, 255},   {128, 64, 0, 255},    {255, 255, 255, 255}, {192, 192, 192, 255},
    {255, 0, 0, 255},     {255, 255, 0, 255},   {0, 255, 0, 255},     {0, 255, 255, 255},
    {0, 0, 255, 255},     {255, 0, 255, 255},   {255, 255, 128, 255}, {128, 255, 255, 255},
    {128, 128, 255, 255}, {255, 0, 128, 255},   {255, 128, 64, 255},  {255, 192, 128, 255},
    {64, 0, 0, 255},      {64, 32, 0, 255},     {64, 64, 0, 255},     {0, 64, 0, 255},
    {0, 64, 64, 255},     {0, 0, 64, 255},      {64, 0, 64, 255},     {32, 32, 32, 255},
    {255, 128, 128, 255}, {255, 255, 192, 255}, {192, 255, 192, 255}, {192, 255, 255, 255},
    {192, 192, 255, 255}, {255, 192, 255, 255}, {192, 128, 64, 255},  {128, 64, 64, 255},
    {64, 128, 64, 255},   {64, 64, 128, 255},   {128, 64, 128, 255},  {0, 0, 0, 0},
};

}  // namespace

ColorsPanel::ColorsPanel() : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 2) {
  set_border_width(3);
  heading_.set_text("Colors");
  heading_.set_xalign(0.0f);
  hint_.set_text("L:FG  M:BG  R:store FG");
  hint_.set_xalign(0.0f);
  hint_.set_line_wrap(true);
  hint_.set_max_width_chars(18);
  grid_.set_row_spacing(1);
  grid_.set_column_spacing(1);
  pack_start(heading_, Gtk::PACK_SHRINK);
  pack_start(grid_, Gtk::PACK_SHRINK);
  pack_start(hint_, Gtk::PACK_SHRINK);
  palette_.assign(kDefaultPalette, kDefaultPalette + 48);
  for (int i = 0; i < static_cast<int>(palette_.size()); ++i) {
    add_swatch(i, palette_[static_cast<std::size_t>(i)].a == 0 ? "Transparent"
                                                              : "Palette color");
  }
}

void ColorsPanel::set_colors(Color fg, Color bg) {
  fg_ = fg;
  bg_ = bg;
}

void ColorsPanel::add_swatch(int index, const char* tip) {
  auto* area = Gtk::manage(new Gtk::DrawingArea());
  area->set_size_request(16, 16);
  area->set_tooltip_text(tip);
  area->add_events(Gdk::BUTTON_PRESS_MASK);
  area->signal_draw().connect(
      [this, area, index](const Cairo::RefPtr<Cairo::Context>& cr) {
        return on_swatch_draw(area, cr, index);
      });
  area->signal_button_press_event().connect(
      [this, index](GdkEventButton* event) { return on_swatch_press(event, index); });
  grid_.attach(*area, col_, row_, 1, 1);
  areas_.push_back(area);
  col_ += 1;
  if (col_ >= 8) {
    col_ = 0;
    row_ += 1;
  }
}

bool ColorsPanel::on_swatch_draw(Gtk::DrawingArea* area, const Cairo::RefPtr<Cairo::Context>& cr,
                                 int index) {
  const Color color = palette_[static_cast<std::size_t>(index)];
  const int w = area->get_allocated_width();
  const int h = area->get_allocated_height();
  if (color.a == 0) {
    cr->set_source_rgb(0.82, 0.82, 0.82);
    cr->rectangle(0, 0, w, h);
    cr->fill();
    cr->set_source_rgb(0.62, 0.62, 0.62);
    cr->rectangle(0, 0, w / 2, h / 2);
    cr->fill();
    cr->rectangle(w / 2, h / 2, w - w / 2, h - h / 2);
    cr->fill();
  } else {
    cr->set_source_rgb(color.r / 255.0, color.g / 255.0, color.b / 255.0);
    cr->rectangle(0, 0, w, h);
    cr->fill();
  }
  cr->set_source_rgb(0.25, 0.25, 0.25);
  cr->rectangle(0.5, 0.5, w - 1.0, h - 1.0);
  cr->set_line_width(1.0);
  cr->stroke();
  return true;
}

bool ColorsPanel::on_swatch_press(GdkEventButton* event, int index) {
  if (event == nullptr || index < 0 || index >= static_cast<int>(palette_.size())) {
    return false;
  }
  Color& slot = palette_[static_cast<std::size_t>(index)];
  const bool last_transparent = (index == static_cast<int>(palette_.size()) - 1);
  if (event->button == 3 && !last_transparent) {
    slot = fg_;
    if (areas_[static_cast<std::size_t>(index)] != nullptr) {
      areas_[static_cast<std::size_t>(index)]->queue_draw();
    }
    return true;
  }
  if (!on_swatch) {
    return false;
  }
  if (event->button == 1) {
    on_swatch(slot, (event->state & GDK_SHIFT_MASK) != 0);
    return true;
  }
  if (event->button == 2) {
    on_swatch(slot, true);
    return true;
  }
  if (event->button == 3 && last_transparent) {
    on_swatch(slot, true);
    return true;
  }
  return false;
}

}  // namespace lundukepaint
