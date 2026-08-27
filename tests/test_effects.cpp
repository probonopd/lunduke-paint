// SPDX-License-Identifier: GPL-3.0-or-later

#include "doc/commands_pixels.hpp"
#include "doc/document.hpp"
#include "raster/effects.hpp"

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
    std::fprintf(stderr, "test_effects: %s\n", msg);
    return 1;
  }
  return 0;
}

Color get(const std::uint8_t* buf, int stride, int x, int y) {
  const std::uint8_t* p = buf + static_cast<std::size_t>(y) * static_cast<std::size_t>(stride) +
                          static_cast<std::size_t>(x) * 4;
  return {p[0], p[1], p[2], p[3]};
}

void set(std::uint8_t* buf, int stride, int x, int y, Color c) {
  std::uint8_t* p = buf + static_cast<std::size_t>(y) * static_cast<std::size_t>(stride) +
                    static_cast<std::size_t>(x) * 4;
  p[0] = c.r;
  p[1] = c.g;
  p[2] = c.b;
  p[3] = c.a;
}

}  // namespace

int main() {
  int errors = 0;

  // Invert a 2×2 buffer; alpha stays.
  {
    const int w = 2;
    const int h = 2;
    const int stride = w * 4;
    std::vector<std::uint8_t> buf(static_cast<std::size_t>(stride * h), 0);
    set(buf.data(), stride, 0, 0, Color{0, 0, 0, 255});
    set(buf.data(), stride, 1, 0, Color{255, 128, 0, 200});
    set(buf.data(), stride, 0, 1, Color{10, 20, 30, 0});
    set(buf.data(), stride, 1, 1, Color{255, 255, 255, 255});
    brushpad::invert_rgba(buf.data(), w, h, stride);
    errors += expect(get(buf.data(), stride, 0, 0) == (Color{255, 255, 255, 255}),
                     "invert black -> white");
    errors += expect(get(buf.data(), stride, 1, 0) == (Color{0, 127, 255, 200}),
                     "invert orange keeps alpha");
    errors += expect(get(buf.data(), stride, 0, 1) == (Color{245, 235, 225, 0}),
                     "invert dark keeps zero alpha");
    errors += expect(get(buf.data(), stride, 1, 1) == (Color{0, 0, 0, 255}), "invert white -> black");
  }

  // Grayscale is a luma mix.
  {
    const int w = 1;
    const int h = 1;
    std::vector<std::uint8_t> buf{255, 0, 0, 255};
    brushpad::grayscale_rgba(buf.data(), w, h, 4);
    errors += expect(buf[0] == buf[1] && buf[1] == buf[2], "grayscale equal channels");
    errors += expect(buf[0] > 0 && buf[0] < 255, "red luma is mid-dark");
    errors += expect(buf[3] == 255, "grayscale keeps alpha");
  }

  // Invert as an undoable command on a tiny document.
  {
    auto doc = Document::create(4, 3, Color{40, 50, 60, 255}, "Background");
    Layer& layer = doc->layers().active_layer();
    Layer before(layer.width(), layer.height(), Color::transparent(), "before");
    before.copy_from(layer);
    brushpad::invert_rgba(layer.pixels(), layer.width(), layer.height(), layer.stride());
    Layer after(layer.width(), layer.height(), Color::transparent(), "after");
    after.copy_from(layer);
    auto cmd = PixelPatchCommand::from_layers(before, after, Rect{0, 0, 4, 3}, "Invert");
    errors += expect(!cmd->empty(), "invert produced a patch");
    doc->history().commit_applied(std::move(cmd));
    errors += expect(layer.pixel(0, 0) == (Color{215, 205, 195, 255}), "invert applied");
    doc->history().undo(*doc);
    errors += expect(layer.pixel(0, 0) == (Color{40, 50, 60, 255}), "undo invert");
    doc->history().redo(*doc);
    errors += expect(layer.pixel(0, 0) == (Color{215, 205, 195, 255}), "redo invert");
  }

  if (errors != 0) {
    std::fprintf(stderr, "test_effects: %d failure(s)\n", errors);
    return 1;
  }
  std::printf("test_effects: ok\n");
  return 0;
}
