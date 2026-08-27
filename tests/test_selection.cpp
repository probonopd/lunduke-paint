// SPDX-License-Identifier: GPL-3.0-or-later

#include "doc/commands_pixels.hpp"
#include "doc/document.hpp"
#include "doc/selection.hpp"

#include <cstdio>
#include <vector>

namespace {

using brushpad::Color;
using brushpad::Document;
using brushpad::Layer;
using brushpad::PixelPatchCommand;
using brushpad::Rect;
using brushpad::Selection;

int expect(bool cond, const char* msg) {
  if (!cond) {
    std::fprintf(stderr, "test_selection: %s\n", msg);
    return 1;
  }
  return 0;
}

}  // namespace

int main() {
  int errors = 0;
  auto doc = Document::create(8, 6, Color::white(), "Background");
  Layer& layer = doc->layers().active_layer();
  layer.fill_rect({1, 1, 3, 3}, Color::black());

  Selection sel;
  errors += expect(sel.empty(), "new selection is empty");
  sel.set_rect({1, 1, 3, 3});
  errors += expect(sel.contains(2, 2), "rect contains interior");
  errors += expect(!sel.contains(0, 0), "rect excludes outside");
  sel.invert(8, 6);
  errors += expect(sel.contains(0, 0), "invert includes outside");
  errors += expect(!sel.contains(2, 2), "invert excludes hole");
  sel.invert(8, 6);
  errors += expect(sel.contains(2, 2), "invert twice restores rect");

  // Delete (fill with transparency) is undoable.
  Layer before(layer.width(), layer.height(), Color::transparent(), "before");
  before.copy_from(layer);
  Rect dirty{};
  brushpad::fill_selection(layer, sel, Color::transparent(), &dirty);
  errors += expect(layer.pixel(2, 2).fully_transparent(), "delete punches transparency");
  errors += expect(layer.pixel(0, 0) == Color::white(), "outside selection stays");
  auto cmd = PixelPatchCommand::from_layers(before, layer, dirty, "Delete");
  errors += expect(!cmd->empty(), "delete produced a patch");
  doc->history().commit_applied(std::move(cmd));
  doc->history().undo(*doc);
  errors += expect(layer.pixel(2, 2) == Color::black(), "undo delete restores pixels");
  doc->history().redo(*doc);
  errors += expect(layer.pixel(2, 2).fully_transparent(), "redo delete punches again");

  // Crop helper: copy the selection rect.
  int cw = 0;
  int ch = 0;
  std::vector<std::uint8_t> copied;
  sel.set_rect({1, 1, 3, 3});
  // After redo the hole is transparent; recopy from a filled source.
  layer.fill_rect({1, 1, 3, 3}, Color{10, 20, 30, 255});
  brushpad::copy_selection_rgba(layer, sel, 8, 6, cw, ch, copied);
  errors += expect(cw == 3 && ch == 3, "copy size matches rect");
  errors += expect(copied.size() == 3u * 3u * 4u, "copy buffer size");
  errors += expect(copied[0] == 10 && copied[1] == 20 && copied[2] == 30 && copied[3] == 255,
                   "copied top-left pixel");

  if (errors != 0) {
    std::fprintf(stderr, "test_selection: %d failure(s)\n", errors);
    return 1;
  }
  std::printf("test_selection: ok\n");
  return 0;
}
