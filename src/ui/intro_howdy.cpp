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
// h: lead-in, open rounded loop, vertical stem to baseline, n-arch to baseline.
// Open loops; round caps/joins; monoline; kPen 13.5.
// Source of truth: data/howdy.svg (not a live skeleton/potrace pipeline).
constexpr double kPen = 13.50;
constexpr double kDesignHeight = 250.0;
constexpr Point kStart{8.00, 146.60};
constexpr std::array<Curve, 34> kCurves{{
    {{25.08, 116.90}, {48.84, 55.26}, {71.12, 21.85}},
    {{89.69, 7.00}, {102.31, 3.28}, {115.68, 14.42}},
    {{124.59, 40.41}, {126.08, 88.68}, {124.59, 125.81}},
    {{123.11, 151.80}, {121.62, 168.88}, {120.14, 176.30}},
    {{129.05, 159.22}, {175.09, 110.96}, {208.51, 94.62}},
    {{230.79, 99.82}, {241.93, 136.95}, {234.50, 176.30}},
    {{245.64, 166.65}, {256.78, 129.52}, {260.49, 103.53}},
    {{271.63, 99.82}, {290.20, 122.09}, {282.77, 159.22}},
    {{277.57, 181.50}, {260.49, 183.73}, {249.35, 162.94}},
    {{245.64, 136.95}, {264.21, 114.67}, {286.49, 118.38}},
    {{305.05, 122.09}, {323.62, 129.52}, {338.47, 140.66}},
    {{349.61, 166.65}, {342.18, 196.35}, {308.76, 200.07}},
    {{282.77, 200.07}, {260.49, 181.50}, {267.92, 151.80}},
    {{286.49, 129.52}, {327.33, 140.66}, {357.03, 124.32}},
    {{364.46, 133.23}, {368.17, 166.65}, {383.03, 186.70}},
    {{397.88, 201.55}, {410.50, 159.22}, {414.96, 127.29}},
    {{420.16, 110.96}, {432.78, 159.22}, {452.09, 186.70}},
    {{468.43, 203.78}, {484.77, 159.22}, {496.65, 125.81}},
    {{509.27, 122.09}, {516.70, 133.23}, {521.90, 151.80}},
    {{516.70, 174.08}, {531.55, 192.64}, {557.54, 191.15}},
    {{579.82, 188.93}, {590.96, 162.94}, {585.76, 133.23}},
    {{581.31, 114.67}, {561.26, 103.53}, {538.98, 107.24}},
    {{520.41, 110.96}, {512.99, 133.23}, {524.13, 155.51}},
    {{542.69, 162.94}, {564.97, 140.66}, {576.11, 103.53}},
    {{583.54, 73.83}, {583.54, 51.55}, {578.34, 44.12}},
    {{573.88, 44.12}, {576.11, 81.25}, {578.34, 125.81}},
    {{579.82, 159.22}, {590.96, 185.21}, {609.53, 186.70}},
    {{624.38, 186.70}, {637.75, 201.55}, {654.09, 186.70}},
    {{667.45, 176.30}, {672.65, 140.66}, {665.22, 124.32}},
    {{660.03, 103.53}, {670.42, 144.37}, {665.22, 192.64}},
    {{661.51, 229.77}, {646.66, 259.47}, {620.67, 265.41}},
    {{598.39, 268.38}, {593.19, 237.19}, {609.53, 203.78}},
    {{622.90, 177.79}, {646.66, 155.51}, {672.65, 146.60}},
    {{687.50, 142.14}, {694.93, 144.37}, {689.73, 146.60}},
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
