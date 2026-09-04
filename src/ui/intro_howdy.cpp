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

// Dense centerline fitted to refs/howdy-reference-exact.png:
// threshold → DT-ridge Dijkstra through writing-order checkpoints →
// Chaikin + B-spline polish. Exact JPEG letterforms; monoline ~20° italic;
// one continuous left→right stroke (not the rejected 0.3-5 noisy Chaikin trace).
constexpr double kPen = 7.00;
constexpr double kDesignHeight = 250.0;
constexpr std::array<Point, 180> kPoly{{
    {69.79, 126.73}, {75.59, 123.06}, {81.33, 118.92}, {86.16, 114.16}, {89.26, 108.61},
    {90.36, 102.20}, {90.57, 95.14}, {91.23, 87.65}, {93.27, 80.35}, {96.91, 74.49},
    {102.24, 71.43}, {107.51, 72.50}, {108.59, 78.75}, {105.95, 86.45}, {102.21, 92.60},
    {98.42, 98.11}, {95.85, 103.78}, {98.32, 107.74}, {108.28, 108.03}, {112.39, 111.15},
    {109.84, 117.92}, {107.17, 125.68}, {109.05, 132.08}, {114.05, 135.89}, {119.51, 136.09},
    {123.79, 132.46}, {127.23, 126.33}, {130.39, 119.24}, {133.87, 112.71}, {138.24, 108.28},
    {144.03, 107.42}, {150.38, 110.44}, {154.90, 116.25}, {155.28, 123.58}, {152.84, 129.31},
    {151.81, 128.82}, {154.92, 121.40}, {161.30, 115.24}, {167.74, 115.37}, {170.72, 119.66},
    {171.35, 126.40}, {171.46, 133.81}, {178.11, 136.67}, {175.15, 135.54}, {170.83, 131.15},
    {171.27, 124.25}, {172.95, 117.04}, {172.23, 112.61}, {165.75, 114.80}, {158.38, 115.81},
    {154.35, 119.43}, {153.04, 126.24}, {153.62, 127.04}, {156.10, 120.85}, {161.28, 114.73},
    {169.20, 115.42}, {171.57, 120.49}, {172.22, 117.26}, {174.21, 109.96}, {173.98, 110.81},
    {171.63, 119.76}, {170.20, 127.64}, {172.27, 132.91}, {177.42, 135.68}, {183.49, 133.87},
    {188.49, 126.67}, {191.31, 120.74}, {191.54, 124.02}, {192.17, 132.29}, {197.10, 136.12},
    {204.03, 134.53}, {208.96, 129.83}, {211.79, 123.35}, {213.49, 116.14}, {216.70, 113.41},
    {222.82, 117.02}, {226.85, 119.98}, {228.77, 126.47}, {230.73, 133.16}, {234.83, 136.50},
    {241.01, 135.69}, {247.77, 131.81}, {253.61, 125.96}, {257.02, 119.23}, {256.49, 112.72},
    {251.60, 107.58}, {247.86, 105.15}, {252.05, 106.49}, {257.43, 111.15}, {255.97, 118.02},
    {254.32, 125.33}, {255.50, 131.83}, {260.26, 135.97}, {267.14, 135.26}, {274.23, 131.48},
    {279.34, 132.99}, {279.85, 128.40}, {278.55, 121.32}, {277.79, 126.10}, {272.93, 132.09},
    {266.15, 135.86}, {260.07, 135.16}, {256.04, 130.29}, {254.49, 123.39}, {255.66, 116.49},
    {258.76, 110.01}, {262.45, 103.64}, {263.75, 100.87}, {259.87, 107.58}, {257.04, 113.33},
    {255.11, 120.77}, {255.02, 128.22}, {257.81, 133.58}, {263.28, 135.68}, {269.81, 134.43},
    {275.70, 129.82}, {279.39, 122.19}, {280.76, 116.40}, {280.44, 118.86}, {277.77, 125.44},
    {273.13, 132.09}, {267.20, 135.84}, {261.00, 135.14}, {256.19, 131.04}, {254.41, 124.82},
    {255.79, 117.62}, {258.64, 110.41}, {261.27, 104.08}, {263.30, 98.35}, {265.59, 91.90},
    {268.83, 83.77}, {271.15, 78.19}, {269.58, 81.56}, {267.08, 88.27}, {265.07, 94.75},
    {262.78, 101.19}, {259.70, 107.76}, {256.60, 114.44}, {254.71, 121.09}, {255.25, 127.61},
    {258.94, 133.47}, {264.64, 136.63}, {270.77, 134.73}, {275.95, 127.68}, {279.31, 120.71},
    {280.03, 119.75}, {278.98, 126.13}, {279.92, 133.25}, {285.15, 135.72}, {292.29, 133.71},
    {298.20, 128.31}, {300.34, 125.80}, {299.49, 134.71}, {304.40, 137.94}, {311.77, 135.23},
    {313.99, 133.84}, {308.16, 136.57}, {301.55, 136.61}, {299.71, 130.43}, {301.71, 121.92},
    {304.36, 115.73}, {304.55, 116.22}, {302.23, 122.78}, {299.90, 130.67}, {298.42, 137.87},
    {297.32, 144.56}, {296.09, 150.97}, {294.20, 157.33}, {291.16, 163.89}, {286.70, 170.54},
    {282.47, 174.68}, {283.30, 173.35}, {289.82, 168.15}, {292.68, 161.95}, {293.62, 155.12},
    {295.50, 148.19}, {299.02, 142.32}, {304.55, 138.38}, {311.04, 135.41}, {316.93, 131.84}
}};

double dist(Point a, Point b) { return std::hypot(b.x - a.x, b.y - a.y); }

const std::vector<Point>& points() {
  static const std::vector<Point> v(kPoly.begin(), kPoly.end());
  return v;
}

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
  if (us <= 0) { return 0; }
  if (us >= kDurationUs) { return 1; }
  return double(us) / kDurationUs;
}

bool finished(long us) { return us >= kDurationUs; }

double path_length() {
  static const double n = []() {
    double r = 0;
    const auto& p = points();
    for (std::size_t i = 1; i < p.size(); ++i) { r += dist(p[i - 1], p[i]); }
    return r;
  }();
  return n;
}

double revealed_length(double p) { return std::clamp(p, 0.0, 1.0) * path_length(); }

int curve_count() { return 24; }

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
}  // namespace brushpad
