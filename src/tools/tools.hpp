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

}  // namespace brushpad

#endif
