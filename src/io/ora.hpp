// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_IO_ORA_HPP
#define BRUSHPAD_IO_ORA_HPP

#include "doc/layer.hpp"

#include <string>
#include <vector>

namespace brushpad {

class Document;

struct LoadedOra {
  int width = 0;
  int height = 0;
  std::vector<LayerSnapshot> layers;  // bottom → top
  std::string error;
  bool warn_size = false;
  bool warn_layers = false;
  bool ok() const { return error.empty() && width > 0 && height > 0 && !layers.empty(); }
};

LoadedOra load_ora(const std::string& path);
bool save_ora(const std::string& path, const Document& document, std::string& error);

}  // namespace brushpad

#endif
