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
constexpr double kPen = 12.50;
constexpr double kDesignHeight = 250.0;
constexpr Point kStart{30.00, 131.14};
constexpr std::array<Curve, 31> kCurves{{
    {{32.97, 116.29}, {44.85, 83.62}, {59.70, 53.91}},
    {{68.61, 33.12}, {83.46, 19.46}, {98.32, 27.18}},
    {{110.20, 33.12}, {114.95, 65.80}, {109.01, 101.44}},
    {{103.07, 128.17}, {92.37, 148.96}, {85.25, 156.09}},
    {{89.40, 137.08}, {107.23, 101.44}, {133.96, 92.53}},
    {{154.75, 86.59}, {169.60, 104.41}, {174.35, 122.23}},
    {{169.60, 137.08}, {178.51, 157.87}, {199.30, 160.84}},
    {{220.09, 162.63}, {236.13, 145.99}, {237.92, 125.20}},
    {{239.10, 107.38}, {226.04, 92.53}, {205.24, 90.75}},
    {{190.39, 89.56}, {178.51, 101.44}, {180.29, 118.07}},
    {{193.36, 128.17}, {214.15, 110.35}, {234.95, 106.19}},
    {{240.89, 113.32}, {243.86, 140.05}, {255.74, 156.09}},
    {{267.62, 167.97}, {277.72, 134.11}, {281.28, 108.57}},
    {{285.44, 95.50}, {295.54, 134.11}, {310.98, 156.09}},
    {{324.05, 169.75}, {337.12, 134.11}, {346.63, 107.38}},
    {{356.73, 104.41}, {362.67, 113.32}, {366.82, 128.17}},
    {{362.67, 145.99}, {374.55, 160.84}, {395.34, 159.65}},
    {{413.16, 157.87}, {422.07, 137.08}, {417.91, 113.32}},
    {{414.35, 98.47}, {398.31, 89.56}, {380.49, 92.53}},
    {{365.64, 95.50}, {359.70, 113.32}, {368.61, 131.14}},
    {{383.46, 137.08}, {401.28, 119.26}, {410.19, 89.56}},
    {{416.13, 65.80}, {416.13, 42.03}, {411.97, 31.34}},
    {{408.41, 42.03}, {410.19, 71.74}, {411.97, 107.38}},
    {{413.16, 134.11}, {422.07, 154.90}, {436.92, 156.09}},
    {{448.80, 156.09}, {459.50, 167.97}, {472.56, 156.09}},
    {{483.26, 147.77}, {487.42, 119.26}, {481.48, 106.19}},
    {{477.32, 95.50}, {485.63, 128.17}, {481.48, 166.78}},
    {{478.51, 196.49}, {466.62, 223.22}, {447.61, 225.00}},
    {{430.98, 226.19}, {426.23, 199.46}, {438.11, 172.72}},
    {{447.61, 154.90}, {466.62, 137.08}, {487.42, 129.95}},
    {{499.30, 126.39}, {505.24, 128.17}, {501.08, 129.95}},
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
