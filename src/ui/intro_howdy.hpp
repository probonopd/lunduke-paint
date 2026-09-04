// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_UI_INTRO_HOWDY_HPP
#define BRUSHPAD_UI_INTRO_HOWDY_HPP

#include <cairo.h>

// Cursive howdy: dense centerline fitted to refs/howdy-reference-exact.png letterforms.
// The overlay is revealed by travelled length over exactly one second.
// Nothing here touches the document or any font.
namespace brushpad {
namespace intro {

inline const char* text() { return "howdy"; }

constexpr long kDurationUs = 1000000;

double progress(long elapsed_us);
bool finished(long elapsed_us);

// Flattened path length in the design coordinate system.
double path_length();
// Distance travelled at clamped animation progress in [0, 1].
double revealed_length(double progress_value);
// Logical letter-construction segment count (path is a dense polished polyline).
int curve_count();

struct Ink {
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;
};

bool measure(int size_px, Ink& ink);
void draw(cairo_t* cr, double x, double y, int size_px, double progress_value);

}  // namespace intro
}  // namespace brushpad

#endif
