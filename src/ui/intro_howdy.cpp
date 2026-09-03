// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/intro_howdy.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
namespace brushpad { namespace intro { namespace {
struct Point { double x; double y; };
struct Curve { Point c1; Point c2; Point end; };
// One continuous trail in h-o-w-d-y order: looped h, oval o, two w valleys,
// oval/tall d, and y with a looped descender. Connectors stay in the pen path.
constexpr Point kStart{18.0,155.0};
constexpr std::array<Curve,24> kCurves{{
{{22,140},{30,80},{36,20}},{{40,4},{62,2},{70,22}},
{{76,38},{48,70},{42,145}},{{50,115},{72,88},{92,95}},
{{112,102},{108,130},{118,148}},{{132,160},{148,168},{168,155}},
{{190,140},{198,108},{180,95}},{{160,82},{132,100},{138,130}},
{{144,155},{175,158},{200,140}},{{210,150},{215,160},{228,155}},
{{240,148},{242,105},{250,98}},{{255,115},{252,158},{270,155}},
{{285,152},{288,105},{300,98}},{{312,115},{308,160},{328,162}},
{{352,164},{368,130},{358,100}},{{348,78},{318,90},{322,125}},
{{328,150},{350,145},{362,90}},{{372,30},{368,6},{384,12}},
{{398,18},{392,80},{388,148}},{{395,165},{410,168},{425,150}},
{{438,132},{440,108},{448,100}},{{452,130},{450,175},{438,200}},
{{428,216},{408,208},{412,188}},{{416,178},{430,176},{442,182}}
}};
constexpr std::array<Curve,0> kTail{{}};
constexpr int kSteps=24;
constexpr double kDesignHeight=214.0;
Point cubic(Point p,const Curve& q,double t){double u=1-t,a=u*u*u,b=3*u*u*t,c=3*u*t*t,d=t*t*t;return {a*p.x+b*q.c1.x+c*q.c2.x+d*q.end.x,a*p.y+b*q.c1.y+c*q.c2.y+d*q.end.y};}
const std::vector<Point>& points(){static const std::vector<Point> v=[](){std::vector<Point> o; Point from=kStart;o.push_back(from);auto add=[&](const auto& curves){for(const Curve& q:curves){Point start=from;for(int i=1;i<=kSteps;++i)o.push_back(cubic(start,q,double(i)/kSteps));from=q.end;}};add(kCurves);add(kTail);return o;}();return v;}
double dist(Point a,Point b){return std::hypot(b.x-a.x,b.y-a.y);}
void bounds(double& ax,double& ay,double& bx,double& by){const auto& p=points();ax=bx=p[0].x;ay=by=p[0].y;for(Point q:p){ax=std::min(ax,q.x);ay=std::min(ay,q.y);bx=std::max(bx,q.x);by=std::max(by,q.y);}}
} // namespace

double progress(long us){if(us<=0)return 0; if(us>=kDurationUs)return 1; return double(us)/kDurationUs;}
bool finished(long us){return us>=kDurationUs;}
double path_length(){static const double n=[](){double r=0;const auto& p=points();for(std::size_t i=1;i<p.size();++i)r+=dist(p[i-1],p[i]);return r;}();return n;}
double revealed_length(double p){return std::clamp(p,0.0,1.0)*path_length();}
int curve_count(){return int(kCurves.size());}
bool measure(int size,Ink& ink){ink={};if(size<1)return false;double ax,ay,bx,by;bounds(ax,ay,bx,by);double s=size/kDesignHeight,pen=std::max(2.0,5.5*s);ink.width=int(std::ceil((bx-ax)*s+pen));ink.height=int(std::ceil((by-ay)*s+pen));return ink.width>0&&ink.height>0;}
void draw(cairo_t* cr,double x,double y,int size,double p){if(!cr||size<1||p<=0)return;const auto& path=points();double ax,ay,bx,by;bounds(ax,ay,bx,by);double s=size/kDesignHeight,left=revealed_length(p);cairo_save(cr);cairo_set_source_rgb(cr,0,0,0);cairo_set_line_width(cr,std::max(2.0,5.5*s));cairo_set_line_cap(cr,CAIRO_LINE_CAP_ROUND);cairo_set_line_join(cr,CAIRO_LINE_JOIN_ROUND);cairo_move_to(cr,x+(path[0].x-ax)*s,y+(path[0].y-ay)*s);for(std::size_t i=1;i<path.size()&&left>0;++i){double len=dist(path[i-1],path[i]);Point end=path[i];if(left<len){double t=left/len;end={path[i-1].x+(path[i].x-path[i-1].x)*t,path[i-1].y+(path[i].y-path[i-1].y)*t};}cairo_line_to(cr,x+(end.x-ax)*s,y+(end.y-ay)*s);left-=len;}cairo_stroke(cr);cairo_restore(cr);}
} }
