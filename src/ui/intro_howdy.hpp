// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_UI_INTRO_HOWDY_HPP
#define BRUSHPAD_UI_INTRO_HOWDY_HPP

#include <cairo.h>

// The startup greeting: "howdy" written on a brand new empty canvas in the
// calligraphic URW "Z003" face (the metric-compatible clone of ITC Zapf
// Chancery that ships in urw-base35), revealed left to right over exactly one
// second. Nothing here touches the document: the word is painted by the canvas
// draw handler as a transient overlay only.
//
// Limitation: Pango hands out filled glyph outlines, not pen paths, so this is
// a left-to-right ink reveal of the finished word rather than a true
// stroke-by-stroke pen animation.
namespace brushpad {
namespace intro {

inline const char* text() {
  return "howdy";
}

inline const char* family() {
  return "Z003";
}

// Exactly one second, in microseconds (g_get_monotonic_time units).
constexpr long kDurationUs = 1000000;

// Width of the soft leading edge, in pixels, that stands in for a pen tip.
constexpr double kFeatherPx = 10.0;

// 0.0 at the first frame, 1.0 once the second is up. Clamped both ways.
double progress(long elapsed_us);

// True once the animation has run its full second.
bool finished(long elapsed_us);

// How much of the word (plus the feather) is revealed at this progress.
double reveal_width(double progress_value, double ink_width);

// True when fontconfig/Pango can actually resolve the family.
bool font_available(const char* family_name = family());

struct Ink {
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;
};

// Ink extents of "howdy" at size_px. False when Pango could not lay it out.
bool measure(int size_px, Ink& ink, const char* family_name = family());

// Paints the partially revealed word in black with its ink top-left at (x, y).
// Draws nothing for progress <= 0.
void draw(cairo_t* cr, double x, double y, int size_px, double progress_value,
          const char* family_name = family());

}  // namespace intro
}  // namespace brushpad

#endif
