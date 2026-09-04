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

// Centerline fitted to refs/howdy-reference-exact.jpeg:
// threshold → skeleton → writing-order Dijkstra landmarks (incl. d-stem
// retrace) → heavy B-spline → cubics. Exact JPEG letterforms; one
// continuous L→R stroke; not noisy 0.3-5 Chaikin.
constexpr double kPen = 27.8552;
constexpr Point kStart{21.35, 154.96};
constexpr std::array<Curve, 20> kCurves{{
    {{65.91, 159.29}, {78.85, 121.83}, {80.15, 85.04}},
    {{89.89, 40.33}, {129.32, 22.92}, {106.03, 80.50}},
    {{108.02, 106.84}, {40.78, 167.05}, {85.95, 124.79}},
    {{134.34, 84.39}, {120.24, 145.33}, {128.24, 170.80}},
    {{165.74, 184.63}, {159.27, 121.31}, {183.90, 177.38}},
    {{211.60, 176.10}, {248.90, 101.38}, {259.18, 155.58}},
    {{278.31, 121.70}, {181.98, 133.75}, {241.10, 131.54}},
    {{267.19, 151.53}, {267.98, 185.47}, {315.74, 167.15}},
    {{344.32, 179.07}, {363.21, 88.23}, {382.85, 152.97}},
    {{393.12, 200.09}, {451.52, 158.23}, {445.86, 123.68}},
    {{429.86, 131.26}, {446.92, 201.08}, {490.26, 166.94}},
    {{506.90, 106.68}, {499.69, 161.87}, {463.93, 174.39}},
    {{420.50, 162.94}, {453.40, 111.50}, {463.11, 84.35}},
    {{488.50, 22.65}, {451.95, 91.58}, {448.15, 119.58}},
    {{428.22, 170.95}, {469.52, 174.06}, {499.79, 158.09}},
    {{507.85, 182.84}, {602.31, 171.94}, {542.59, 176.26}},
    {{555.18, 102.12}, {525.87, 175.66}, {560.66, 175.77}},
    {{515.50, 181.44}, {536.35, 243.10}, {504.40, 255.88}},
    {{448.47, 251.55}, {511.81, 206.52}, {530.01, 189.87}},
    {{520.46, 132.05}, {554.05, 202.64}, {583.93, 160.30}},
}};

constexpr int kSteps = 28;
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
  if (us <= 0) return 0;
  if (us >= kDurationUs) return 1;
  return double(us) / kDurationUs;
}

bool finished(long us) { return us >= kDurationUs; }

double path_length() {
  static const double n = []() {
    double r = 0;
    const auto& p = points();
    for (std::size_t i = 1; i < p.size(); ++i) r += dist(p[i - 1], p[i]);
    return r;
  }();
  return n;
}

double revealed_length(double p) { return std::clamp(p, 0.0, 1.0) * path_length(); }

int curve_count() { return int(kCurves.size()); }

bool measure(int size, Ink& ink) {
  ink = {};
  if (size < 1) return false;
  double ax, ay, bx, by;
  bounds(ax, ay, bx, by);
  const double s = size / kDesignHeight;
  const double pen = std::max(2.0, kPen * s);
  ink.width = int(std::ceil((bx - ax) * s + pen));
  ink.height = int(std::ceil((by - ay) * s + pen));
  return ink.width > 0 && ink.height > 0;
}

void draw(cairo_t* cr, double x, double y, int size, double p) {
  if (!cr || size < 1 || p <= 0) return;
  const auto& path = points();
  double ax, ay, bx, by;
  bounds(ax, ay, bx, by);
  const double s = size / kDesignHeight;
  double left = revealed_length(p);
  cairo_save(cr);
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_set_line_width(cr, std::max(2.0, kPen * s));
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
}  // namespace lundukepaint
