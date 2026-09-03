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
// 25 curves — not a PNG skeleton / Chaikin / Catmull-Rom auto-trace.
// Flat editor proportions: looped h + n-arch, slanted o, twin U w, stem d, looped y.
constexpr Point kStart{40.0, 150.0};
constexpr std::array<Curve, 25> kCurves{{
    // h: short lead-in from baseline into a tall NARROW rounded loop
    {{32, 115}, {34, 58}, {48, 50}},
    {{60, 46}, {64, 58}, {62, 96}},
    // h: almost-vertical stem down to the baseline (x held ~62, y to 150)
    {{62, 128}, {62, 148}, {62, 150}},
    // h: THEN one n-arch — right along the floor, up to x-height, back to baseline
    {{80, 150}, {88, 104}, {104, 100}},
    {{120, 98}, {128, 128}, {126, 150}},
    // connector into o
    {{140, 148}, {152, 142}, {164, 132}},
    // o: smooth slanted oval; high bridge into w
    {{152, 150}, {168, 156}, {186, 148}},
    {{206, 138}, {212, 118}, {196, 106}},
    {{178, 94}, {158, 104}, {166, 122}},
    {{178, 138}, {198, 132}, {214, 116}},
    // w: two distinct U valleys to baseline, middle peak at x-height
    {{216, 138}, {220, 150}, {238, 150}},
    {{252, 150}, {256, 104}, {270, 100}},
    {{284, 100}, {288, 150}, {306, 150}},
    {{322, 150}, {330, 112}, {334, 108}},
    // d: o-like bowl
    {{324, 128}, {330, 154}, {356, 152}},
    {{380, 150}, {388, 122}, {374, 108}},
    {{360, 96}, {344, 104}, {348, 122}},
    // d: tall straight slanted stem (up, then down — not a giant loop)
    {{360, 132}, {372, 124}, {380, 108}},
    {{392, 85}, {404, 58}, {410, 55}},
    {{404, 75}, {394, 125}, {386, 150}},
    // connector into y
    {{400, 154}, {416, 144}, {428, 124}},
    // y: U-cup then smooth descender loop
    {{438, 138}, {444, 152}, {460, 152}},
    {{476, 152}, {484, 120}, {488, 110}},
    {{494, 130}, {498, 172}, {472, 180}},
    {{452, 186}, {448, 160}, {488, 154}},
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
