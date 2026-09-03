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
// monoline. Tall looped h, simple oval o, two midline valleys for w,
// d with oval body + retraced stem (no top loop), y with looped descender.
// One continuous trail in writing order.
constexpr Point kStart{28.0, 156.0};
constexpr std::array<Curve, 23> kCurves{{
    // h: tall thin loop, stem down, rounded shoulder into o
    {{20, 100}, {16, 16}, {42, 4}},
    {{74, 0}, {88, 28}, {80, 158}},
    {{80, 108}, {108, 86}, {134, 122}},
    // o: slightly wide oval sitting on the baseline (kept)
    {{118, 148}, {132, 164}, {166, 162}},
    {{202, 160}, {210, 120}, {184, 98}},
    {{160, 80}, {132, 96}, {148, 124}},
    {{168, 146}, {194, 136}, {212, 114}},
    // w: two clean valleys, exit at midline into d
    {{220, 142}, {230, 168}, {254, 160}},
    {{276, 152}, {284, 108}, {292, 98}},
    {{300, 112}, {304, 168}, {330, 160}},
    {{352, 152}, {362, 116}, {370, 106}},
    // d: o-like bowl, then tall retraced stem (colinear controls = no top loop)
    {{354, 138}, {360, 168}, {398, 164}},
    {{434, 160}, {442, 116}, {420, 95}},
    {{400, 78}, {378, 88}, {382, 118}},
    {{400, 130}, {418, 120}, {422, 95}},
    {{422, 60}, {422, 28}, {422, 4}},
    {{422, 40}, {422, 100}, {422, 162}},
    {{430, 172}, {452, 158}, {468, 124}},
    // y: rounded cup, then a looped descender (kept)
    {{478, 144}, {484, 168}, {508, 164}},
    {{530, 160}, {538, 112}, {544, 100}},
    {{550, 140}, {556, 210}, {528, 230}},
    {{502, 248}, {480, 234}, {488, 208}},
    {{496, 190}, {516, 188}, {528, 200}},
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
