// SPDX-License-Identifier: GPL-3.0-or-later

#include "doc/workspace.hpp"

#include <stdexcept>

namespace lundukepaint {

Document& Workspace::active() {
  return at(active_);
}

const Document& Workspace::active() const {
  return at(active_);
}

Document* Workspace::active_ptr() {
  if (active_ < 0 || active_ >= count()) {
    return nullptr;
  }
  return documents_[static_cast<std::size_t>(active_)].get();
}

Document& Workspace::at(int index) {
  if (index < 0 || index >= count()) {
    throw std::out_of_range("document index");
  }
  return *documents_[static_cast<std::size_t>(index)];
}

const Document& Workspace::at(int index) const {
  if (index < 0 || index >= count()) {
    throw std::out_of_range("document index");
  }
  return *documents_[static_cast<std::size_t>(index)];
}

int Workspace::add(std::unique_ptr<Document> document) {
  if (!document) {
    return active_;
  }
  documents_.push_back(std::move(document));
  active_ = count() - 1;
  return active_;
}

void Workspace::set_active(int index) {
  if (index >= 0 && index < count()) {
    active_ = index;
  }
}

void Workspace::replace_active(std::unique_ptr<Document> document) {
  if (!document) {
    return;
  }
  if (active_ < 0 || documents_.empty()) {
    add(std::move(document));
    return;
  }
  documents_[static_cast<std::size_t>(active_)] = std::move(document);
}

void Workspace::close(int index) {
  if (index < 0 || index >= count()) {
    return;
  }
  documents_.erase(documents_.begin() + index);
  if (documents_.empty()) {
    active_ = -1;
    return;
  }
  if (active_ > index) {
    --active_;
  } else if (active_ >= count()) {
    active_ = count() - 1;
  }
}

int Workspace::index_of(const Document* document) const {
  if (document == nullptr) {
    return -1;
  }
  for (int i = 0; i < count(); ++i) {
    if (documents_[static_cast<std::size_t>(i)].get() == document) {
      return i;
    }
  }
  return -1;
}

bool Workspace::is_placeholder(int index) const {
  if (index < 0 || index >= count()) {
    return false;
  }
  const Document& doc = at(index);
  return !doc.dirty() && doc.path().empty() && doc.history().count() == 0 &&
         doc.layers().count() == 1;
}

}  // namespace lundukepaint
