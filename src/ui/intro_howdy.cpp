// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/intro_howdy.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace brushpad {
namespace intro {
namespace {

struct Point {
  double x;
  double y;
};

struct Curve {
  Point c1;
  Point c2;
  Point end;
};

// Hand-fit smooth cubics matching refs/howdy-editor-ref.png letterforms.
// 22 curves — not a PNG skeleton / Chaikin / Catmull-Rom auto-trace.
// Same hand: ~20° italic, shared baseline / x-height / ascender, even stroke.
// h = tall narrow loop + vertical stem to baseline + ONE n-arch.
// w = two U valleys on the baseline, middle peak at x-height.
// d = oval bowl joined to a tall stem that grows out of the bowl's right side.
constexpr Point kStart{27.0, 136.0};
constexpr std::array<Curve, 22> kCurves{{
    // h: lead-in up LEFT of tall narrow loop
    {{29, 116}, {48, 84}, {62, 79}},
    // h: rounded loop top onto the right / stem
    {{74, 68}, {92, 70}, {85, 94}},
    // h: STEM nearly vertical — hits baseline (not an extra hump)
    {{85, 118}, {83, 142}, {82, 151}},
    // h: n-arch grows from the stem's baseline point (no floor gap)
    {{95, 125}, {108, 98}, {118, 98}},
    // h: n-arch down into o
    {{128, 98}, {126, 148}, {128, 135}},
    // o: bottom of slanted oval
    {{135, 153}, {151, 154}, {164, 137}},
    // o: right side up to x-height
    {{173, 121}, {170, 97}, {150, 97}},
    // o: left close
    {{130, 97}, {120, 114}, {130, 131}},
    // o: high bridge into w
    {{144, 142}, {172, 101}, {194, 110}},
    // w: first U down to baseline
    {{203, 131}, {203, 151}, {212, 151}},
    // w: first U up — MIDDLE PEAK at x-height (same as o top)
    {{220, 150}, {218, 98}, {223, 98}},
    // w: second U down to baseline
    {{228, 98}, {237, 151}, {251, 151}},
    // w: second U up to x-height, into d
    {{268, 150}, {280, 99}, {303, 98}},
    // d: enter bowl from w's high exit, around oval (do not close as a separate o)
    {{292, 118}, {294, 154}, {326, 152}},
    // d: up the RIGHT side of the bowl — that edge IS the stem
    {{350, 152}, {360, 130}, {358, 104}},
    // d: continue up that same right edge; small loop at h-loop height
    {{360, 86}, {366, 68}, {370, 72}},
    // d: retrace down ON the bowl's right edge to baseline (joined d, not o+l)
    {{360, 90}, {358, 120}, {356, 150}},
    // d: connector into y
    {{372, 150}, {387, 122}, {398, 116}},
    // y: U-valley down to baseline
    {{406, 135}, {410, 152}, {416, 148}},
    // y: U-valley up to x-height
    {{422, 145}, {429, 99}, {425, 98}},
    // y: descender
    {{425, 122}, {406, 176}, {414, 174}},
    // y: loop and exit flourish
    {{428, 174}, {454, 118}, {474, 133}},
}};

constexpr int kSteps = 24;
constexpr double kDesignHeight = 250.0;

Point cubic(Point p, const Curve& q, double t) {
  const double u = 1.0 - t;
  const double a = u * u * u;
  const double b = 3.0 * u * u * t;
  const double c = 3.0 * u * t * t;
  const double d = t * t * t;
  return {a * p.x + b * q.c1.x + c * q.c2.x + d * q.end.x,
          a * p.y + b * q.c1.y + c * q.c2.y + d * q.end.y};
}

const std::vector<Point>& points() {
  static const std::vector<Point> v = []() {
    std::vector<Point> out;
    Point from = kStart;
    out.push_back(from);
    for (const Curve& q : kCurves) {
      const Point start = from;
      for (int i = 1; i <= kSteps; ++i) {
        out.push_back(cubic(start, q, double(i) / kSteps));
      }
      from = q.end;
    }
    return out;
  }();
  return v;
}

double dist(Point a, Point b) { return std::hypot(b.x - a.x, b.y - a.y); }

void bounds(double& ax, double& ay, double& bx, double& by) {
  const auto& p = points();
  ax = bx = p[0].x;
  ay = by = p[0].y;
  for (Point q : p) {
    ax = std::min(ax, q.x);
    ay = std::min(ay, q.y);
    bx = std::max(bx, q.x);
    by = std::max(by, q.y);
  }
}

}  // namespace

double progress(long us) {
  if (us <= 0) {
    return 0;
  }
  if (us >= kDurationUs) {
    return 1;
  }
  return double(us) / kDurationUs;
}

bool finished(long us) { return us >= kDurationUs; }

double path_length() {
  static const double n = []() {
    double r = 0;
    const auto& p = points();
    for (std::size_t i = 1; i < p.size(); ++i) {
      r += dist(p[i - 1], p[i]);
    }
    return r;
  }();
  return n;
}

double revealed_length(double p) { return std::clamp(p, 0.0, 1.0) * path_length(); }

int curve_count() { return int(kCurves.size()); }

bool measure(int size, Ink& ink) {
  ink = {};
  if (size < 1) {
    return false;
  }
  double ax, ay, bx, by;
  bounds(ax, ay, bx, by);
  const double s = size / kDesignHeight;
  const double pen = std::max(2.0, 5.5 * s);
  ink.width = int(std::ceil((bx - ax) * s + pen));
  ink.height = int(std::ceil((by - ay) * s + pen));
  return ink.width > 0 && ink.height > 0;
}

void draw(cairo_t* cr, double x, double y, int size, double p) {
  if (!cr || size < 1 || p <= 0) {
    return;
  }
  const auto& path = points();
  double ax, ay, bx, by;
  bounds(ax, ay, bx, by);
  const double s = size / kDesignHeight;
  double left = revealed_length(p);
  cairo_save(cr);
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_set_line_width(cr, std::max(2.0, 5.5 * s));
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
  cairo_move_to(cr, x + (path[0].x - ax) * s, y + (path[0].y - ay) * s);
  for (std::size_t i = 1; i < path.size() && left > 0; ++i) {
    const double len = dist(path[i - 1], path[i]);
    Point end = path[i];
    if (left < len) {
      const double t = left / len;
      end = {path[i - 1].x + (path[i].x - path[i - 1].x) * t,
             path[i - 1].y + (path[i].y - path[i - 1].y) * t};
    }
    cairo_line_to(cr, x + (end.x - ax) * s, y + (end.y - ay) * s);
    left -= len;
  }
  cairo_stroke(cr);
  cairo_restore(cr);
}

}  // namespace intro
}  // namespace brushpad
