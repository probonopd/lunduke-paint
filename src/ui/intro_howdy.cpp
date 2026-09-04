// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/intro_howdy.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace lundukepaint {
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

// Hand-authored howdy cubics from data/howdy.svg (design space).
// Letterforms matched to refs/howdy-reference-exact.jpeg; one L→R stroke.
// Open teardrop h / open y descender; round caps/joins; monoline.
// Source of truth: data/howdy.svg (not a live skeleton/potrace pipeline).
constexpr double kPen = 13.50;
constexpr double kDesignHeight = 250.0;
constexpr Point kStart{8.00, 140.66};
constexpr std::array<Curve, 31> kCurves{{
    {{11.71, 122.10}, {26.57, 81.25}, {45.13, 44.12}},
    {{56.27, 18.13}, {74.84, 1.05}, {93.40, 10.70}},
    {{108.25, 18.13}, {114.20, 58.98}, {106.77, 103.53}},
    {{99.34, 136.95}, {85.98, 162.94}, {77.06, 171.86}},
    {{82.26, 148.09}, {104.54, 103.53}, {137.96, 92.39}},
    {{163.95, 84.97}, {182.52, 107.25}, {188.46, 129.53}},
    {{182.52, 148.09}, {193.66, 174.08}, {219.65, 177.80}},
    {{245.64, 180.02}, {265.69, 159.23}, {267.92, 133.24}},
    {{269.41, 110.96}, {253.07, 92.39}, {227.08, 90.17}},
    {{208.51, 88.68}, {193.66, 103.53}, {195.88, 124.33}},
    {{212.22, 136.95}, {238.21, 114.67}, {264.21, 109.47}},
    {{271.63, 118.39}, {275.35, 151.80}, {290.20, 171.86}},
    {{305.05, 186.71}, {317.68, 144.38}, {322.13, 112.44}},
    {{327.33, 96.11}, {339.95, 144.38}, {359.26, 171.86}},
    {{375.60, 188.94}, {391.94, 144.38}, {403.82, 110.96}},
    {{416.45, 107.25}, {423.87, 118.39}, {429.07, 136.95}},
    {{423.87, 159.23}, {438.72, 177.80}, {464.72, 176.31}},
    {{487.00, 174.08}, {498.13, 148.09}, {492.94, 118.39}},
    {{488.48, 99.82}, {468.43, 88.68}, {446.15, 92.39}},
    {{427.58, 96.11}, {420.16, 118.39}, {431.30, 140.66}},
    {{449.86, 148.09}, {472.14, 125.81}, {483.28, 88.68}},
    {{490.71, 58.98}, {490.71, 29.27}, {485.51, 15.90}},
    {{481.05, 29.27}, {483.28, 66.40}, {485.51, 110.96}},
    {{487.00, 144.38}, {498.13, 170.37}, {516.70, 171.86}},
    {{531.55, 171.86}, {544.92, 186.71}, {561.26, 171.86}},
    {{574.63, 161.46}, {579.82, 125.81}, {572.40, 109.47}},
    {{567.20, 96.11}, {577.60, 136.95}, {572.40, 185.22}},
    {{568.68, 222.35}, {553.83, 255.77}, {530.07, 258.00}},
    {{509.27, 259.49}, {503.33, 226.07}, {518.19, 192.65}},
    {{530.07, 170.37}, {553.83, 148.09}, {579.82, 139.18}},
    {{594.68, 134.72}, {602.10, 136.95}, {596.90, 139.18}},
}};

constexpr int kSteps = 32;

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

double curve_length(Point from, const Curve& q) {
  double r = 0;
  Point prev = from;
  for (int i = 1; i <= kSteps; ++i) {
    const Point cur = cubic(from, q, double(i) / kSteps);
    r += dist(prev, cur);
    prev = cur;
  }
  return r;
}

}  // namespace

double progress(long us) {
  if (us <= 0) { return 0; }
  if (us >= kDurationUs) { return 1; }
  return double(us) / kDurationUs;
}

bool finished(long us) { return us >= kDurationUs; }

double path_length() {
  static const double n = []() {
    double r = 0;
    Point from = kStart;
    for (const Curve& q : kCurves) {
      r += curve_length(from, q);
      from = q.end;
    }
    return r;
  }();
  return n;
}

double revealed_length(double p) { return std::clamp(p, 0.0, 1.0) * path_length(); }

int curve_count() { return int(kCurves.size()); }

bool measure(int size, Ink& ink) {
  ink = {};
  if (size < 1) { return false; }
  double ax, ay, bx, by;
  bounds(ax, ay, bx, by);
  const double s = size / kDesignHeight;
  const double pen = std::max(2.0, kPen * s);
  ink.width = int(std::ceil((bx - ax) * s + pen));
  ink.height = int(std::ceil((by - ay) * s + pen));
  return ink.width > 0 && ink.height > 0;
}

void draw(cairo_t* cr, double x, double y, int size, double p) {
  if (!cr || size < 1 || p <= 0) { return; }
  double ax, ay, bx, by;
  bounds(ax, ay, bx, by);
  const double s = size / kDesignHeight;
  double left = revealed_length(p);
  cairo_save(cr);
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_set_line_width(cr, std::max(2.0, kPen * s));
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
  Point from = kStart;
  cairo_move_to(cr, x + (from.x - ax) * s, y + (from.y - ay) * s);
  for (const Curve& q : kCurves) {
    if (left <= 0) { break; }
    const double clen = curve_length(from, q);
    if (left >= clen - 1e-9) {
      cairo_curve_to(cr,
                     x + (q.c1.x - ax) * s, y + (q.c1.y - ay) * s,
                     x + (q.c2.x - ax) * s, y + (q.c2.y - ay) * s,
                     x + (q.end.x - ax) * s, y + (q.end.y - ay) * s);
      left -= clen;
      from = q.end;
      continue;
    }
    Point prev = from;
    for (int i = 1; i <= kSteps && left > 0; ++i) {
      const Point cur = cubic(from, q, double(i) / kSteps);
      const double seg = dist(prev, cur);
      Point end = cur;
      if (left < seg) {
        const double t = left / seg;
        end = {prev.x + (cur.x - prev.x) * t, prev.y + (cur.y - prev.y) * t};
      }
      cairo_line_to(cr, x + (end.x - ax) * s, y + (end.y - ay) * s);
      left -= seg;
      prev = cur;
    }
    break;
  }
  cairo_stroke(cr);
  cairo_restore(cr);
}

}  // namespace intro
}  // namespace lundukepaint
