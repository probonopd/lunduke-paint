// SPDX-License-Identifier: GPL-3.0-or-later

#include "tools/tool.hpp"

#include "doc/document.hpp"

#include <cmath>

namespace lundukepaint {

class PickerTool : public Tool {
public:
  const char* id() const override { return "picker"; }
  const char* name() const override { return "Color picker"; }
  char shortcut() const override { return 'C'; }
  const char* hint() const override { return "Picker: left sets FG, right sets BG"; }

  void on_press(CanvasEvent event) override;
  void on_motion(CanvasEvent /*event*/) override {}
  void on_release(CanvasEvent /*event*/) override {}
  void on_cancel() override {}
};

void PickerTool::on_press(CanvasEvent event) {
  if (host_ == nullptr || (event.button != 1 && event.button != 3)) {
    return;
  }
  const int x = static_cast<int>(std::floor(event.x));
  const int y = static_cast<int>(std::floor(event.y));
  const Color color = host_->sample_canvas(x, y);
  if (event.button == 3) {
    host_->document().set_background(color);
  } else {
    host_->document().set_foreground(color);
  }
  host_->return_to_previous_tool();
}

Tool* create_picker_tool() {
  return new PickerTool();
}

}  // namespace lundukepaint
