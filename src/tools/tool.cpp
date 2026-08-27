// SPDX-License-Identifier: GPL-3.0-or-later

#include "tools/tool.hpp"

#include "doc/document.hpp"

namespace brushpad {

Color Tool::stroke_color(unsigned button) const {
  if (host_ == nullptr) {
    return Color::black();
  }
  if (button == 3) {
    return host_->document().background();
  }
  return host_->document().foreground();
}

}  // namespace brushpad
