// SPDX-License-Identifier: GPL-3.0-or-later

#include "doc/commands_pixels.hpp"
#include "doc/document.hpp"
#include "raster/fill.hpp"
#include "raster/stroke.hpp"

#include <algorithm>
#include <cstdio>
#include <vector>

namespace {

using brushpad::Color;
using brushpad::Document;
using brushpad::Layer;
using brushpad::PixelPatchCommand;
using brushpad::Rect;

int expect(bool cond, const char* msg) {
  if (!cond) {
    std::fprintf(stderr, "test_history: %s\n", msg);
    return 1;
  }
  return 0;
}

std::vector<std::uint8_t> snapshot(const Layer& layer) {
  std::vector<std::uint8_t> out(static_cast<std::size_t>(layer.stride()) *
                                static_cast<std::size_t>(layer.height()));
  std::copy(layer.pixels(), layer.pixels() + out.size(), out.begin());
  return out;
}

bool same_pixels(const Layer& a, const std::vector<std::uint8_t>& b) {
  const std::size_t n = static_cast<std::size_t>(a.stride()) * static_cast<std::size_t>(a.height());
  return std::equal(a.pixels(), a.pixels() + n, b.begin());
}

}  // namespace

int main() {
  int errors = 0;
  auto doc = Document::create(16, 12, Color::white(), "Background");
  Layer& layer = doc->layers().active_layer();
  const auto original = snapshot(layer);

  // Stroke command: pencil scribble, one history item, undo invertibility.
  {
    Layer before(layer.width(), layer.height(), Color::transparent(), "before");
    before.copy_from(layer);
    Rect dirty{};
    brushpad::stroke_pencil(layer.pixels(), layer.width(), layer.height(), layer.stride(), 1, 1, 10,
                            7, 2, Color::black(), &dirty);
    Layer after(layer.width(), layer.height(), Color::transparent(), "after");
    after.copy_from(layer);
    auto cmd = PixelPatchCommand::from_layers(before, after, dirty, "Pencil stroke");
    errors += expect(!cmd->empty(), "stroke produced a dirty patch");
    // Command already applied on the layer; record without re-applying.
    doc->history().commit_applied(std::move(cmd));
    errors += expect(doc->history().can_undo(), "can undo stroke");
    const auto stroked = snapshot(layer);
    errors += expect(!same_pixels(layer, original), "stroke changed pixels");

    doc->history().undo(*doc);
    errors += expect(same_pixels(layer, original), "undo stroke restores pixels");
    errors += expect(doc->history().can_redo(), "can redo stroke");

    doc->history().redo(*doc);
    errors += expect(same_pixels(layer, stroked), "redo stroke matches after");
    doc->history().undo(*doc);
  }

  // Fill command undo invertibility.
  {
    Layer before(layer.width(), layer.height(), Color::transparent(), "before");
    before.copy_from(layer);
    Rect dirty{};
    brushpad::flood_fill(layer.pixels(), layer.width(), layer.height(), layer.stride(), 0, 0,
                         Color{0, 128, 255, 255}, 0, &dirty);
    Layer after(layer.width(), layer.height(), Color::transparent(), "after");
    after.copy_from(layer);
    auto cmd = PixelPatchCommand::from_layers(before, after, dirty, "Flood fill");
    errors += expect(!cmd->empty(), "fill produced a dirty patch");
    doc->history().commit_applied(std::move(cmd));
    const auto filled = snapshot(layer);
    errors += expect(layer.pixel(0, 0) == (Color{0, 128, 255, 255}), "fill wrote seed");

    doc->history().undo(*doc);
    errors += expect(same_pixels(layer, original), "undo fill restores original");
    doc->history().redo(*doc);
    errors += expect(same_pixels(layer, filled), "redo fill matches after");
  }

  if (errors != 0) {
    std::fprintf(stderr, "test_history: %d failure(s)\n", errors);
    return 1;
  }
  std::printf("test_history: ok\n");
  return 0;
}
