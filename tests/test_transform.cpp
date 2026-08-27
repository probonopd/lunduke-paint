// SPDX-License-Identifier: GPL-3.0-or-later

#include "doc/commands_image.hpp"
#include "doc/document.hpp"
#include "raster/transform.hpp"

#include <cstdio>
#include <vector>

namespace {

using brushpad::Color;
using brushpad::Document;
using brushpad::Layer;
using brushpad::LayerBufferCommand;

int expect(bool cond, const char* msg) {
  if (!cond) {
    std::fprintf(stderr, "test_transform: %s\n", msg);
    return 1;
  }
  return 0;
}

}  // namespace

int main() {
  int errors = 0;

  // 2×2 rotate 90 CW: [A B]    [C A]
  //                   [C D] -> [D B]
  {
    const int w = 2;
    const int h = 2;
    std::vector<std::uint8_t> src(16, 0);
    auto set = [&](int x, int y, Color c) {
      std::uint8_t* p = src.data() + static_cast<std::size_t>((y * w + x) * 4);
      p[0] = c.r;
      p[1] = c.g;
      p[2] = c.b;
      p[3] = c.a;
    };
    auto get = [&](const std::vector<std::uint8_t>& buf, int x, int y, int bw) {
      const std::uint8_t* p = buf.data() + static_cast<std::size_t>((y * bw + x) * 4);
      return Color{p[0], p[1], p[2], p[3]};
    };
    const Color A{1, 0, 0, 255};
    const Color B{2, 0, 0, 255};
    const Color C{3, 0, 0, 255};
    const Color D{4, 0, 0, 255};
    set(0, 0, A);
    set(1, 0, B);
    set(0, 1, C);
    set(1, 1, D);
    std::vector<std::uint8_t> dest(16, 0);
    brushpad::rotate_90_cw(src.data(), w, h, w * 4, dest.data(), h * 4);
    errors += expect(get(dest, 0, 0, 2) == C, "CW top-left is C");
    errors += expect(get(dest, 1, 0, 2) == A, "CW top-right is A");
    errors += expect(get(dest, 0, 1, 2) == D, "CW bottom-left is D");
    errors += expect(get(dest, 1, 1, 2) == B, "CW bottom-right is B");

    std::vector<std::uint8_t> flipped = src;
    brushpad::flip_h(flipped.data(), w, h, w * 4);
    errors += expect(get(flipped, 0, 0, 2) == B, "flip H top-left is B");
    errors += expect(get(flipped, 1, 0, 2) == A, "flip H top-right is A");

    auto doc = Document::create(2, 2, Color::transparent(), "Background");
    doc->layers().active_layer().set_pixels(2, 2, src.data(), 8);
    auto cmd = LayerBufferCommand::from_buffers("Rotate 90", 2, 2, src.data(), 8, 2, 2, dest.data(),
                                                8, 0);
    doc->commit(std::move(cmd));
    errors += expect(doc->layers().active_layer().pixel(0, 0) == C, "apply rotate");
    doc->undo();
    errors += expect(doc->layers().active_layer().pixel(0, 0) == A, "undo rotate restores");
    doc->redo();
    errors += expect(doc->layers().active_layer().pixel(1, 1) == B, "redo rotate");

    std::vector<std::uint8_t> after_flip = src;
    brushpad::flip_v(after_flip.data(), w, h, w * 4);
    auto cmd2 = LayerBufferCommand::from_buffers("Flip vertical", 2, 2, dest.data(), 8, 2, 2,
                                                 after_flip.data(), 8, 0);
    doc->commit(std::move(cmd2));
    doc->undo();
    errors += expect(doc->layers().active_layer().pixel(0, 0) == C, "undo flip restores rotate");
  }

  if (errors != 0) {
    std::fprintf(stderr, "test_transform: %d failure(s)\n", errors);
    return 1;
  }
  std::printf("test_transform: ok\n");
  return 0;
}
