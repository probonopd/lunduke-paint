// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_APP_PREFERENCES_HPP
#define BRUSHPAD_APP_PREFERENCES_HPP

#include "raster/types.hpp"

#include <string>

namespace brushpad {

class Preferences {
public:
  int default_width = kDefaultWidth;
  int default_height = kDefaultHeight;
  int undo_limit = kDefaultUndoDepth;
  Color checker_light{209, 209, 209, 255};
  Color checker_dark{158, 158, 158, 255};
  int grid_threshold = 400;

  void load();
  bool save() const;

  static std::string config_dir();
  static std::string config_path();
};

}  // namespace brushpad

#endif
