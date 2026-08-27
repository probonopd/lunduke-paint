// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_TOOLS_TOOLS_HPP
#define BRUSHPAD_TOOLS_TOOLS_HPP

namespace brushpad {

class Tool;

Tool* create_pencil_tool();
Tool* create_brush_tool();
Tool* create_eraser_tool();
Tool* create_picker_tool();
Tool* create_fill_tool();
Tool* create_rect_select_tool();
Tool* create_line_tool();
Tool* create_rectangle_tool();
Tool* create_ellipse_tool();
Tool* create_color_eraser_tool();
Tool* create_spray_tool();
Tool* create_rounded_rect_tool();
Tool* create_polyline_tool();
Tool* create_polygon_tool();
Tool* create_curve_tool();
Tool* create_lasso_tool();
Tool* create_ellipse_select_tool();

}  // namespace brushpad

#endif
