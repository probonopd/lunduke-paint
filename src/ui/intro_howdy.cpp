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
// monoline. One hand throughout: looped h and looped d (same ascender
// family), oval o, two rounded U-valleys for w, y with a looped descender.
// h's shoulder is a wide arch that lands on the baseline before o.
constexpr Point kStart{28.0, 156.0};
constexpr std::array<Curve, 25> kCurves{{
    // h: tall rounded ascender loop (up, round top, down to baseline)
    {{18, 90}, {14, 8}, {38, 2}},
    {{52, -6}, {40, 72}, {66, 160}},
    // h shoulder: wide rounded arch from baseline up to x-height
    {{74, 118}, {82, 80}, {100, 76}},
    // rounded crown of the arch
    {{116, 70}, {126, 74}, {134, 96}},
    // right stem of the arch: vertical drop all the way to baseline
    {{136, 124}, {136, 150}, {136, 162}},
    // sit on baseline, then connector into o
    {{150, 166}, {166, 158}, {178, 146}},
    // o: oval on the baseline; exit high to w
    {{164, 170}, {184, 174}, {206, 160}},
    {{232, 144}, {238, 108}, {214, 88}},
    {{186, 72}, {158, 90}, {172, 122}},
    {{194, 148}, {222, 136}, {240, 110}},
    // w: two rounded U-shaped valleys on the baseline
    {{246, 128}, {252, 162}, {270, 162}},
    {{288, 162}, {294, 92}, {304, 82}},
    {{312, 82}, {318, 162}, {338, 162}},
    {{356, 162}, {366, 114}, {370, 102}},
    // d: o-like bowl
    {{352, 138}, {358, 168}, {396, 164}},
    {{432, 160}, {440, 116}, {418, 95}},
    {{398, 78}, {376, 88}, {380, 118}},
    // d: tall rounded ascender loop matching h (not a retraced stem)
    {{408, 70}, {404, 8}, {428, 2}},
    {{442, -6}, {430, 72}, {456, 160}},
    {{460, 170}, {478, 156}, {492, 124}},
    // y: U-cup matching w, then looped descender
    {{502, 144}, {508, 168}, {532, 164}},
    {{554, 160}, {562, 112}, {568, 100}},
    {{574, 140}, {580, 210}, {552, 230}},
    {{526, 248}, {504, 234}, {512, 208}},
    {{520, 190}, {540, 188}, {552, 200}},
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
