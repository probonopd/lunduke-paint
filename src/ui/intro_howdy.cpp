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

// Mac 1984 "hello." spirit: slanted connected cursive, uniform felt-tip
// monoline. Tall looped h with a clear baseline arch, simple oval o,
// sharp two-valley w, d with oval body + retraced stem (no top loop),
// y with looped descender. One continuous trail in writing order.
constexpr Point kStart{26.0, 158.0};
constexpr std::array<Curve, 25> kCurves{{
    // h: tall rounded ascender loop, stem to baseline, wide clear arch to baseline
    {{16, 90}, {10, 8}, {36, 0}},
    {{64, -6}, {94, 42}, {86, 160}},
    // shoulder left: from baseline up to a rounded midline hump
    {{92, 110}, {108, 78}, {128, 88}},
    // shoulder right: more vertical drop to baseline, land before o
    {{138, 98}, {142, 150}, {146, 162}},
    {{150, 162}, {154, 162}, {158, 160}},
    // o: oval on the baseline (entry from h landing)
    {{140, 172}, {160, 174}, {184, 160}},
    {{210, 144}, {216, 108}, {192, 88}},
    {{164, 72}, {136, 90}, {150, 122}},
    {{172, 148}, {200, 136}, {218, 110}},
    // w: sharp two-valley — deep baseline points, pointed midline peak
    {{226, 145}, {232, 178}, {252, 168}},
    {{258, 140}, {260, 84}, {274, 84}},
    {{288, 84}, {290, 178}, {312, 168}},
    {{330, 150}, {350, 112}, {368, 102}},
    // d: o-like bowl, then tall retraced stem (colinear controls = no top loop)
    {{352, 138}, {358, 168}, {396, 164}},
    {{432, 160}, {440, 116}, {418, 95}},
    {{398, 78}, {376, 88}, {380, 118}},
    {{398, 130}, {416, 120}, {420, 95}},
    {{420, 60}, {420, 28}, {420, 4}},
    {{420, 40}, {420, 100}, {420, 162}},
    {{428, 172}, {450, 158}, {466, 124}},
    // y: rounded cup, then a looped descender (kept)
    {{476, 144}, {482, 168}, {506, 164}},
    {{528, 160}, {536, 112}, {542, 100}},
    {{548, 140}, {554, 210}, {526, 230}},
    {{500, 248}, {478, 234}, {486, 208}},
    {{494, 190}, {514, 188}, {526, 200}},
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
