// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef LUNDUKEPAINT_DOC_COMMAND_HPP
#define LUNDUKEPAINT_DOC_COMMAND_HPP

#include "raster/types.hpp"

#include <string>

namespace lundukepaint {

class Document;

class Command {
public:
  virtual ~Command() = default;

  virtual std::string name() const = 0;
  virtual void apply(Document& document) = 0;
  virtual void undo(Document& document) = 0;
  virtual Rect dirty_rect() const = 0;
};

}  // namespace lundukepaint

#endif
