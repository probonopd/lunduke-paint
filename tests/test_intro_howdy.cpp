// SPDX-License-Identifier: GPL-3.0-or-later
//
// Non-GUI half of the startup greeting: the Z003 calligraphic face is really
// available through Pango, "howdy" lays out with sane ink extents, the reveal
// maths runs for exactly one second, and the progressive reveal really does
// paint more ink as time goes by. No display or GTK needed (image surface only).

#include "ui/intro_howdy.hpp"

#include <cairo.h>
#include <cstdio>
#include <cstring>

namespace {

int errors = 0;

void expect(bool cond, const char* msg) {
  if (!cond) {
    std::fprintf(stderr, "test_intro_howdy: %s\n", msg);
    ++errors;
  }
}

// Counts painted pixels and the right-most painted column of a reveal.
void render(double progress, int size_px, int& painted, int& rightmost) {
  const int w = 1200;
  const int h = 400;
  cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
  cairo_t* cr = cairo_create(surface);
  brushpad::intro::draw(cr, 40.0, 40.0, size_px, progress);
  cairo_surface_flush(surface);
  const unsigned char* data = cairo_image_surface_get_data(surface);
  const int stride = cairo_image_surface_get_stride(surface);
  painted = 0;
  rightmost = -1;
  for (int y = 0; y < h; ++y) {
    const unsigned char* row = data + static_cast<std::size_t>(y) * stride;
    for (int x = 0; x < w; ++x) {
      if (row[x * 4 + 3] > 32) {  // alpha
        ++painted;
        if (x > rightmost) {
          rightmost = x;
        }
      }
    }
  }
  cairo_destroy(cr);
  cairo_surface_destroy(surface);
}

}  // namespace

int main() {
  using namespace brushpad::intro;

  expect(std::strcmp(text(), "howdy") == 0, "the greeting is howdy");
  expect(std::strcmp(family(), "Z003") == 0, "the face is Z003");

  // Exactly one second, and clamped either side.
  expect(kDurationUs == 1000000, "duration is exactly 1 s");
  expect(progress(-5) == 0.0, "progress clamps below zero");
  expect(progress(0) == 0.0, "progress starts at zero");
  expect(progress(500000) > 0.49 && progress(500000) < 0.51, "half way at 500 ms");
  expect(progress(999999) < 1.0, "not finished a microsecond early");
  expect(progress(1000000) == 1.0, "finished at exactly 1 s");
  expect(progress(9000000) == 1.0, "progress clamps above one");
  expect(!finished(999999), "finished() false just before 1 s");
  expect(finished(1000000), "finished() true at 1 s");

  // Reveal width is monotonic and covers the whole word (plus the pen feather).
  expect(reveal_width(0.0, 100.0) == 0.0, "nothing revealed at t=0");
  expect(reveal_width(0.5, 100.0) > 0.0 && reveal_width(0.5, 100.0) < 110.1, "half revealed");
  expect(reveal_width(1.0, 100.0) >= 100.0, "everything revealed at t=1");
  double previous = -1.0;
  for (int i = 0; i <= 10; ++i) {
    const double value = reveal_width(static_cast<double>(i) / 10.0, 100.0);
    expect(value >= previous, "reveal width never goes backwards");
    previous = value;
  }

  // The bundled calligraphic face must resolve, or the greeting is skipped.
  const bool have_z003 = font_available();
  expect(have_z003, "Z003 is available through Pango/fontconfig");
  expect(!font_available("No Such Font Family At All"), "a bogus family is reported missing");

  if (have_z003) {
    Ink ink;
    expect(measure(64, ink), "howdy measures at 64 px");
    expect(ink.width > 40 && ink.height > 10, "ink extents are sane");
    Ink bigger;
    expect(measure(128, bigger), "howdy measures at 128 px");
    expect(bigger.width > ink.width, "bigger size means wider ink");
    expect(!measure(0, ink), "a zero size is rejected");

    // The reveal really is progressive: more ink, further right, as time runs.
    int painted_none = 0;
    int right_none = 0;
    render(0.0, 64, painted_none, right_none);
    expect(painted_none == 0, "nothing painted before the animation starts");

    int painted_quarter = 0;
    int right_quarter = 0;
    render(0.25, 64, painted_quarter, right_quarter);
    int painted_half = 0;
    int right_half = 0;
    render(0.5, 64, painted_half, right_half);
    int painted_full = 0;
    int right_full = 0;
    render(1.0, 64, painted_full, right_full);

    expect(painted_quarter > 0, "ink appears early in the animation");
    expect(painted_half > painted_quarter, "more ink at the halfway point");
    expect(painted_full > painted_half, "most ink once the second is up");
    expect(right_quarter < right_half && right_half < right_full,
           "the ink front sweeps left to right");
    expect(right_full >= 40, "the word is drawn from the requested origin");
  }

  if (errors != 0) {
    std::fprintf(stderr, "test_intro_howdy: %d failure(s)\n", errors);
    return 1;
  }
  std::printf("test_intro_howdy: ok\n");
  return 0;
}
