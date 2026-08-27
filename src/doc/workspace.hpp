// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_DOC_WORKSPACE_HPP
#define BRUSHPAD_DOC_WORKSPACE_HPP

#include "doc/document.hpp"

#include <memory>
#include <vector>

namespace brushpad {

class Workspace {
public:
  Workspace() = default;

  int count() const { return static_cast<int>(documents_.size()); }
  int active_index() const { return active_; }

  Document& active();
  const Document& active() const;
  Document* active_ptr();

  Document& at(int index);
  const Document& at(int index) const;

  int add(std::unique_ptr<Document> document);
  void set_active(int index);
  void replace_active(std::unique_ptr<Document> document);
  void close(int index);

  bool is_placeholder(int index) const;
  int index_of(const Document* document) const;

private:
  std::vector<std::unique_ptr<Document>> documents_;
  int active_ = -1;
};

}  // namespace brushpad

#endif
