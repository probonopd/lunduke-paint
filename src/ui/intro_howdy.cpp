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

// Centerline from refs/howdy-reference-exact.png:
// DT-ridge walk → light Chaikin + mild B-spline → 28 long-handle cubics.
// Exact JPEG letterforms preserved; bold monoline; one L→R stroke.
constexpr double kPen = 14.50;
constexpr double kDesignHeight = 250.0;
constexpr Point kStart{28.00, 130.59};
constexpr std::array<Curve, 28> kCurves{{
    {{54.71, 120.14}, {61.94, 97.92}, {61.95, 70.94}},
    {{57.69, 38.42}, {105.89, 27.24}, {85.50, 65.82}},
    {{53.92, 97.01}, {86.46, 94.35}, {95.68, 113.91}},
    {{72.31, 142.30}, {114.35, 164.01}, {123.00, 131.34}},
    {{119.21, 102.96}, {163.56, 85.08}, {168.04, 119.47}},
    {{153.69, 168.89}, {172.20, 89.26}, {195.12, 119.03}},
    {{198.22, 153.03}, {202.81, 157.90}, {195.68, 123.43}},
    {{208.90, 95.36}, {152.74, 113.54}, {164.50, 135.16}},
    {{169.93, 90.16}, {205.81, 141.21}, {198.39, 106.40}},
    {{205.78, 85.87}, {176.70, 164.29}, {212.93, 144.33}},
    {{239.08, 99.38}, {214.86, 148.44}, {247.99, 145.42}},
    {{264.66, 126.88}, {268.69, 92.76}, {289.24, 124.75}},
    {{281.72, 161.49}, {330.50, 147.15}, {334.37, 121.95}},
    {{329.87, 91.37}, {321.74, 85.49}, {335.66, 117.38}},
    {{317.18, 155.63}, {367.44, 143.83}, {375.31, 140.51}},
    {{379.62, 94.43}, {360.87, 170.96}, {336.08, 140.82}},
    {{320.71, 125.94}, {362.00, 68.32}, {341.43, 100.76}},
    {{324.86, 119.77}, {336.12, 165.72}, {364.43, 139.70}},
    {{384.83, 99.34}, {378.42, 126.04}, {355.46, 146.34}},
    {{320.84, 152.19}, {336.79, 107.49}, {345.35, 90.12}},
    {{358.17, 59.02}, {363.24, 42.01}, {348.96, 83.44}},
    {{340.77, 101.92}, {319.68, 138.49}, {348.54, 147.69}},
    {{380.79, 140.61}, {372.54, 96.49}, {376.15, 143.68}},
    {{409.99, 143.06}, {402.66, 125.03}, {420.43, 149.56}},
    {{444.16, 148.65}, {385.11, 146.73}, {416.56, 112.36}},
    {{414.33, 111.28}, {405.87, 156.70}, {400.79, 174.06}},
    {{400.48, 198.26}, {359.07, 229.93}, {393.21, 197.67}},
    {{397.03, 171.96}, {407.59, 148.84}, {434.87, 141.65}},
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
