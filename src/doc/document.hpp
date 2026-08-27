// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BRUSHPAD_DOC_DOCUMENT_HPP
#define BRUSHPAD_DOC_DOCUMENT_HPP

#include "doc/history.hpp"
#include "doc/layer_stack.hpp"
#include "doc/selection.hpp"
#include "raster/types.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace brushpad {

class Document {
public:
  static std::unique_ptr<Document> create(int width, int height, Color background,
                                          std::string layer_name = "Background");

  int width() const { return width_; }
  int height() const { return height_; }
  int dpi() const { return dpi_; }

  const std::string& path() const { return path_; }
  void set_path(std::string path) { path_ = std::move(path); }

  bool dirty() const { return dirty_; }
  void set_dirty(bool dirty);
  void mark_clean();

  Color foreground() const { return fg_; }
  Color background() const { return bg_; }
  void set_foreground(Color color);
  void set_background(Color color);
  void swap_colors();
  void reset_colors();

  LayerStack& layers() { return layers_; }
  const LayerStack& layers() const { return layers_; }

  History& history() { return history_; }
  const History& history() const { return history_; }

  Selection& selection() { return selection_; }
  const Selection& selection() const { return selection_; }

  Color canvas_background() const { return canvas_bg_; }

  void commit(std::unique_ptr<Command> command);
  Rect undo();
  Rect redo();

  void select_all();
  void deselect();
  void invert_selection();
  bool commit_floating(const char* name = "Move selection");
  void delete_selection();
  void duplicate_selection();
  void paste_floating(int x, int y, int w, int h, std::vector<std::uint8_t> rgba);
  void replace_active_buffer(int width, int height, const std::uint8_t* rgba, int stride);

  using ChangedFn = std::function<void()>;
  using InvalidatedFn = std::function<void(Rect)>;

  void set_on_changed(ChangedFn fn) { on_changed_ = std::move(fn); }
  void set_on_invalidated(InvalidatedFn fn) { on_invalidated_ = std::move(fn); }

  void notify_invalidated(Rect rect);
  void notify_changed();

private:
  Document(int width, int height, Color background, std::string layer_name);

  int width_ = kDefaultWidth;
  int height_ = kDefaultHeight;
  int dpi_ = 96;
  std::string path_;
  bool dirty_ = false;
  Color fg_ = Color::black();
  Color bg_ = Color::white();
  Color canvas_bg_ = Color::white();
  LayerStack layers_;
  History history_;
  Selection selection_;
  ChangedFn on_changed_;
  InvalidatedFn on_invalidated_;
};

}  // namespace brushpad

#endif
