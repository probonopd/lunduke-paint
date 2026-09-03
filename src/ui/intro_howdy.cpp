// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/intro_howdy.hpp"

#include <glib.h>
#include <pango/pangocairo.h>

namespace brushpad {
namespace intro {
namespace {

PangoLayout* make_layout(cairo_t* cr, int size_px, const char* family_name) {
  PangoLayout* layout = pango_cairo_create_layout(cr);
  if (layout == nullptr) {
    return nullptr;
  }
  pango_layout_set_text(layout, text(), -1);
  PangoFontDescription* desc = pango_font_description_new();
  pango_font_description_set_family(desc, family_name);
  pango_font_description_set_style(desc, PANGO_STYLE_ITALIC);
  pango_font_description_set_weight(desc, PANGO_WEIGHT_MEDIUM);
  pango_font_description_set_absolute_size(desc, static_cast<double>(size_px) * PANGO_SCALE);
  pango_layout_set_font_description(layout, desc);
  pango_font_description_free(desc);
  return layout;
}

}  // namespace

double progress(long elapsed_us) {
  if (elapsed_us <= 0) {
    return 0.0;
  }
  if (elapsed_us >= kDurationUs) {
    return 1.0;
  }
  return static_cast<double>(elapsed_us) / static_cast<double>(kDurationUs);
}

bool finished(long elapsed_us) {
  return elapsed_us >= kDurationUs;
}

double reveal_width(double progress_value, double ink_width) {
  if (progress_value <= 0.0) {
    return 0.0;
  }
  const double full = ink_width + kFeatherPx;
  if (progress_value >= 1.0) {
    return full;
  }
  return progress_value * full;
}

bool font_available(const char* family_name) {
  if (family_name == nullptr || family_name[0] == '\0') {
    return false;
  }
  PangoFontMap* map = pango_cairo_font_map_get_default();
  if (map == nullptr) {
    return false;
  }
  PangoFontFamily** families = nullptr;
  int count = 0;
  pango_font_map_list_families(map, &families, &count);
  bool found = false;
  for (int i = 0; i < count && !found; ++i) {
    const char* name = pango_font_family_get_name(families[i]);
    if (name != nullptr && g_ascii_strcasecmp(name, family_name) == 0) {
      found = true;
    }
  }
  g_free(families);
  return found;
}

bool measure(int size_px, Ink& ink, const char* family_name) {
  ink = Ink{};
  if (size_px < 1) {
    return false;
  }
  cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
  cairo_t* cr = cairo_create(surface);
  PangoLayout* layout = make_layout(cr, size_px, family_name);
  bool ok = false;
  if (layout != nullptr) {
    PangoRectangle ink_rect{};
    PangoRectangle logical_rect{};
    pango_layout_get_pixel_extents(layout, &ink_rect, &logical_rect);
    if (ink_rect.width > 0 && ink_rect.height > 0) {
      ink.x = ink_rect.x;
      ink.y = ink_rect.y;
      ink.width = ink_rect.width;
      ink.height = ink_rect.height;
      ok = true;
    }
    g_object_unref(layout);
  }
  cairo_destroy(cr);
  cairo_surface_destroy(surface);
  return ok;
}

void draw(cairo_t* cr, double x, double y, int size_px, double progress_value,
          const char* family_name) {
  if (cr == nullptr || size_px < 1 || progress_value <= 0.0) {
    return;
  }
  PangoLayout* layout = make_layout(cr, size_px, family_name);
  if (layout == nullptr) {
    return;
  }
  PangoRectangle ink_rect{};
  PangoRectangle logical_rect{};
  pango_layout_get_pixel_extents(layout, &ink_rect, &logical_rect);
  if (ink_rect.width <= 0 || ink_rect.height <= 0) {
    g_object_unref(layout);
    return;
  }

  cairo_save(cr);
  // Render the word into a group so the leading edge can be feathered like a
  // pen tip instead of a hard vertical cut.
  cairo_push_group(cr);
  cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
  cairo_move_to(cr, x - ink_rect.x, y - ink_rect.y);
  pango_cairo_update_layout(cr, layout);
  pango_cairo_show_layout(cr, layout);
  cairo_pop_group_to_source(cr);

  if (progress_value >= 1.0) {
    cairo_paint(cr);
  } else {
    const double edge = x + reveal_width(progress_value, ink_rect.width);
    cairo_pattern_t* mask = cairo_pattern_create_linear(edge - kFeatherPx, 0.0, edge, 0.0);
    cairo_pattern_add_color_stop_rgba(mask, 0.0, 0.0, 0.0, 0.0, 1.0);
    cairo_pattern_add_color_stop_rgba(mask, 1.0, 0.0, 0.0, 0.0, 0.0);
    cairo_pattern_set_extend(mask, CAIRO_EXTEND_PAD);
    cairo_mask(cr, mask);
    cairo_pattern_destroy(mask);
  }
  cairo_restore(cr);
  g_object_unref(layout);
}

}  // namespace intro
}  // namespace brushpad
