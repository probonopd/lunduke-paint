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
// Open rounded h / open y descender; round caps/joins; monoline.
// Source of truth: data/howdy.svg (not a live skeleton/potrace pipeline).
constexpr double kPen = 13.50;
constexpr double kDesignHeight = 250.0;
constexpr Point kStart{8.00, 146.60};
constexpr std::array<Curve, 32> kCurves{{
    {{17.65, 125.81}, {35.48, 84.97}, {54.78, 42.64}},
    {{67.41, 18.13}, {82.26, 3.28}, {95.63, 5.51}},
    {{111.96, 8.48}, {117.16, 40.41}, {111.96, 81.25}},
    {{106.77, 114.67}, {89.69, 151.80}, {59.98, 174.08}},
    {{41.42, 185.22}, {45.13, 129.52}, {85.97, 99.82}},
    {{111.96, 81.25}, {137.96, 79.77}, {160.23, 96.10}},
    {{175.09, 114.67}, {178.80, 155.51}, {163.95, 174.08}},
    {{152.81, 155.51}, {145.38, 122.09}, {163.95, 96.10}},
    {{186.22, 77.54}, {227.07, 84.97}, {245.63, 118.38}},
    {{256.77, 144.37}, {249.34, 181.50}, {215.93, 185.22}},
    {{189.94, 186.70}, {167.66, 166.65}, {175.09, 136.95}},
    {{193.65, 114.67}, {234.49, 125.81}, {264.20, 109.47}},
    {{271.62, 118.38}, {275.34, 151.80}, {290.19, 171.85}},
    {{305.04, 186.70}, {317.66, 144.37}, {322.12, 112.44}},
    {{327.32, 96.10}, {339.94, 144.37}, {359.25, 171.85}},
    {{375.59, 188.93}, {391.92, 144.37}, {403.81, 110.96}},
    {{416.43, 107.24}, {423.86, 118.38}, {429.05, 136.95}},
    {{423.86, 159.22}, {438.71, 177.79}, {464.70, 176.30}},
    {{486.98, 174.08}, {498.12, 148.09}, {492.92, 118.38}},
    {{488.46, 99.82}, {468.41, 88.68}, {446.13, 92.39}},
    {{427.57, 96.10}, {420.14, 118.38}, {431.28, 140.66}},
    {{449.85, 148.09}, {472.12, 125.81}, {483.26, 88.68}},
    {{490.69, 58.97}, {490.69, 36.70}, {485.49, 29.27}},
    {{481.04, 29.27}, {483.26, 66.40}, {485.49, 110.96}},
    {{486.98, 144.37}, {498.12, 170.36}, {516.68, 171.85}},
    {{531.53, 171.85}, {544.90, 186.70}, {561.24, 171.85}},
    {{574.60, 161.45}, {579.80, 125.81}, {572.38, 109.47}},
    {{567.18, 96.10}, {577.57, 136.95}, {572.38, 185.22}},
    {{568.66, 222.35}, {553.81, 252.05}, {527.82, 257.99}},
    {{505.54, 260.96}, {500.34, 229.77}, {516.68, 196.35}},
    {{530.05, 170.36}, {553.81, 148.09}, {579.80, 139.17}},
    {{594.65, 134.72}, {602.08, 136.95}, {596.88, 139.17}},
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
