// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/toolbox.hpp"

#include <gdkmm/screen.h>
#include <gtkmm/image.h>
#include <gtkmm/stylecontext.h>

namespace brushpad {

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

Toolbox::Toolbox() : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4) {
  ensure_css();
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

  // Name the two swatches without needing a tooltip hover. "FG" sits under the
  // upper-left well, "BG" under the lower-right one, in the same small type as
  // the rest of the classic toolbox column.
  fg_label_.set_halign(Gtk::ALIGN_START);
  bg_label_.set_halign(Gtk::ALIGN_END);
  fg_label_.set_tooltip_text("Foreground color");
  bg_label_.set_tooltip_text("Background color");
  fg_label_.get_style_context()->add_class(toolbox_style::caption_class());
  bg_label_.get_style_context()->add_class(toolbox_style::caption_class());
  well_labels_.set_size_request(56, -1);
  well_labels_.pack_start(fg_label_, Gtk::PACK_SHRINK);
  well_labels_.pack_end(bg_label_, Gtk::PACK_SHRINK);
  pack_start(well_labels_, Gtk::PACK_SHRINK);

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
  const std::string resource =
      "/org/lunduke/LundukePaint/icons/scalable/actions/" + icon_name + ".svg";
  auto* image = Gtk::manage(new Gtk::Image());
  image->set_from_resource(resource);
  image->set_pixel_size(18);
  button->set_image(*image);
  button->set_tooltip_text(tooltip);
  button->set_relief(Gtk::RELIEF_NONE);
  button->set_can_focus(false);
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
