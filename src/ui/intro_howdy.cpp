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

// Cubics FITTED to a centerline TRACE of refs/howdy-editor-ref.png (225x95):
// ink threshold → upscaled skeleton → writing-order waypoint walk → Chaikin
// smooth → Catmull-Rom→Bezier. Geometry from the PNG, not invented letters.
constexpr Point kStart{70.4, 139.2};
constexpr std::array<Curve, 60> kCurves{{
    {{73.8, 136.9}, {85.1, 131}, {90.7, 125.5}},
    {{96.2, 120.1}, {100.8, 113.5}, {103.6, 106.3}},
    {{106.5, 99.2}, {104.3, 86.6}, {107.6, 82.5}},
    {{110.9, 78.4}, {121.1, 78.2}, {123.5, 81.6}},
    {{126, 85}, {121.1, 96.5}, {122.2, 102.9}},
    {{123.4, 109.3}, {129.9, 113.1}, {130.4, 120}},
    {{130.8, 126.9}, {122.6, 140.9}, {125.1, 144.3}},
    {{127.5, 147.7}, {138.4, 142.8}, {145.2, 140.5}},
    {{152, 138.1}, {162.1, 133.1}, {165.9, 130}},
    {{169.6, 126.9}, {167.3, 119.3}, {167.8, 122}},
    {{168.2, 124.8}, {164.6, 141.8}, {168.4, 146.4}},
    {{172.2, 150.9}, {184.8, 152.4}, {190.7, 149.4}},
    {{196.5, 146.5}, {197.8, 133.1}, {203.6, 128.8}},
    {{209.4, 124.4}, {223.9, 124.1}, {225.3, 123.5}},
    {{226.7, 122.9}, {215.4, 126.5}, {212, 125.3}},
    {{208.6, 124.2}, {204.1, 116.4}, {205.1, 116.6}},
    {{206, 116.7}, {212.3, 126.9}, {217.7, 126.2}},
    {{223.1, 125.5}, {233, 112.2}, {237.4, 112.3}},
    {{241.9, 112.4}, {242.8, 120.9}, {244.4, 126.9}},
    {{245.9, 132.9}, {242.8, 146.6}, {246.8, 148.2}},
    {{250.7, 149.8}, {263.5, 138.8}, {268.1, 136.3}},
    {{272.7, 133.8}, {272.2, 137.6}, {274.4, 133.1}},
    {{276.6, 128.6}, {280.8, 111.4}, {281.4, 109.2}},
    {{282, 107.1}, {279, 114.4}, {278, 120.3}},
    {{277, 126.2}, {272.3, 140.5}, {275.4, 144.7}},
    {{278.5, 148.9}, {291.6, 147.5}, {296.5, 145.5}},
    {{301.4, 143.4}, {301.8, 137.8}, {305.1, 132.4}},
    {{308.3, 127}, {310.5, 117.7}, {315.8, 113.1}},
    {{321.1, 108.5}, {330.5, 103.9}, {336.9, 104.9}},
    {{343.3, 105.8}, {352.4, 112.5}, {354.1, 118.8}},
    {{355.8, 125}, {345.2, 137.8}, {347.2, 142.3}},
    {{349.2, 146.8}, {360, 146.1}, {366.1, 145.8}},
    {{372.2, 145.5}, {379.9, 140}, {383.9, 140.4}},
    {{387.9, 140.8}, {389.7, 150.4}, {389.9, 148.2}},
    {{390.2, 146.1}, {385.4, 135.1}, {385.5, 127.6}},
    {{385.6, 120.2}, {393.5, 107.3}, {390.6, 103.4}},
    {{387.6, 99.5}, {368.6, 104.9}, {367.8, 104.5}},
    {{367.1, 104}, {382.9, 97.5}, {386.1, 100.5}},
    {{389.4, 103.5}, {386.8, 121}, {387.5, 122.3}},
    {{388.2, 123.5}, {388.8, 114.4}, {390.5, 108}},
    {{392.2, 101.7}, {396.6, 87.9}, {397.7, 84.3}},
    {{398.7, 80.6}, {398, 81.9}, {396.8, 86.2}},
    {{395.6, 90.6}, {392.5, 102.2}, {390.3, 110.1}},
    {{388.1, 118.1}, {382.7, 128.1}, {383.7, 134.1}},
    {{384.7, 140.2}, {391.1, 147}, {396.4, 146.4}},
    {{401.7, 145.8}, {411.5, 133.9}, {415.5, 130.6}},
    {{419.6, 127.3}, {419.5, 127.4}, {420.5, 126.6}},
    {{421.6, 125.7}, {418.4, 127.9}, {421.9, 125.3}},
    {{425.4, 122.8}, {438.6, 109.7}, {441.7, 111.2}},
    {{444.9, 112.7}, {438.6, 128.1}, {440.7, 134.3}},
    {{442.9, 140.4}, {449.4, 147.9}, {454.7, 148}},
    {{460, 148.2}, {469.4, 137.3}, {472.5, 135}},
    {{475.5, 132.7}, {472.6, 131.9}, {472.8, 134}},
    {{472.9, 136.1}, {473.5, 145.3}, {473.4, 147.4}},
    {{473.2, 149.4}, {473.6, 143.2}, {472, 146.3}},
    {{470.4, 149.5}, {464.5, 165.9}, {463.8, 166.2}},
    {{463.2, 166.5}, {466.9, 152.7}, {468.1, 148.2}},
    {{469.4, 143.7}, {467.8, 139.6}, {471.1, 139}},
    {{474.5, 138.5}, {484, 147.1}, {488.2, 145.1}},
    {{492.3, 143.2}, {494.7, 130.3}, {496.1, 127.3}},
}};

constexpr int kSteps = 12;
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
